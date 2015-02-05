/*
 *   v895.cc  
 *   set 'threshold' and 'enable' registers.
 *   output width is fixed value: 40ns.
 */
#include <iostream>
#include <fstream>
#include <gef/gefcmn_vme.h>
#include <byteswap.h>

static GEF_VME_BUS_HDL    bus_hdl;
static GEF_VME_MASTER_HDL mst_hdl;
static GEF_MAP_HDL        map_hdl;
struct V895_REG {
  volatile GEF_UINT16 *vth[16];
  volatile GEF_UINT16 *width1;// 0-7ch
  volatile GEF_UINT16 *width2;// 8-15ch
  volatile GEF_UINT16 *majth;
  volatile GEF_UINT16 *enable;
};
static struct V895_REG v895;
static const GEF_VME_ADDR_SPACE V895_AM = GEF_VME_ADDR_SPACE_A32;
static const GEF_UINT32 V895_MAP_SIZE   = 0x100;

void vme_open(unsigned int vme_addr);
void vme_close(); 

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
  if(argc!=2){
    std::cout<<"Usage: v895 [param.txt]"<<std::endl;
    return 0;
  }
  ////////// open param file
  std::ifstream ifs(argv[1]);
  if(!ifs) {
    std::cout<<"#E cannot open file ... "<<argv[1]<<std::endl;
    return 0;
  }

  ////////// read file and setting
  unsigned int vme_addr = 0;
  std::string line;
  while( ifs && std::getline(ifs, line) ) {
    if(line[0]=='#') continue;
    if(line.substr(0,3)=="VME"){
      sscanf(line.c_str(),"VME %x",&vme_addr);
      std::cout<<"# V895: "<<std::hex<<std::showbase<<vme_addr<<std::endl;
      unsigned int Vth[16]    = {};
      unsigned int Enable[16] = {};
      while( ifs && std::getline(ifs, line) ) {
	if(line[0]=='#') continue;
	if(line.substr(0,3)=="END"){
	  vme_open(vme_addr);
	  ////////// Output Width,  0-255: 5-40ns, non-linear
	  *(v895.width1) = __bswap_16(0xff);
	  *(v895.width2) = __bswap_16(0xff);
	  ////////// Majority threshold
	  *(v895.majth)  = __bswap_16(0xff);
	  ////////// Pattern of inhibit
	  unsigned int enable = 0;
	  for(int i=0;i<16;i++) enable = enable|(Enable[i]<<i);
	  *(v895.enable) = __bswap_16(enable);
	  ////////// Discriminator thresholds
	  //std::cout<<"Channel\tVth.\tEnable"<<std::endl;
	  for(int i=0;i<16;i++){
	    std::cout<<std::dec<<i<<"\t"<<Vth[i]<<"\t"<<Enable[i]<<std::endl;
	    *(v895.vth[i]) = __bswap_16(Vth[i]);
	  }
	  vme_close();
	  break;
	}//if(END)
	unsigned int p1,p2,p3;
	if( sscanf(line.c_str(),"%d %d %d", &p1, &p2, &p3)==3 ){
	  if(p1>=16){
	    std::cerr<<"#W channel is too much: "<<p1<<std::endl;
	    continue;
	  }
	  if(p2>=255){
	    std::cerr<<"#W threshold is too much: "<<p2<<"/255"<<std::endl;
	    p2 = 255;
	  }
	  Vth[p1]    = p2 & 0xfff;
	  Enable[p1] = p3 & 0x1;
	}//if(param)
      }//while(getline)
    }//if(VME)
  }//while(getline)
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// internal functions
/////////////////////////////////////////////////////////////////////////////////////////////
void vme_open(unsigned int vme_addr)
{
  int status = 0;
  status = gefVmeOpen(&bus_hdl);
  if( status != GEF_STATUS_SUCCESS ){
    std::cout<<"#E gefVmeOpen() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
  GEF_VME_ADDR addr_param = {
    0x00000000,                     //upoper
    vme_addr & 0xffffffff,          //lower
    V895_AM,                        //addr_space
    GEF_VME_2ESST_RATE_INVALID,     //vme_2esst_rate
    GEF_VME_ADDR_MODE_DEFAULT,      //addr_mode
    GEF_VME_TRANSFER_MODE_SCT,      //transfer_mode
    GEF_VME_BROADCAST_ID_DISABLE,   //broadcast_id
    GEF_VME_TRANSFER_MAX_DWIDTH_32, //transfer_max_dwidth
    GEF_VME_WND_EXCLUSIVE           //flags
  };
  GEF_MAP_PTR ptr;
  status = gefVmeCreateMasterWindow( bus_hdl, &addr_param, V895_MAP_SIZE, &mst_hdl );
  if( status != GEF_STATUS_SUCCESS ){
    std::cout<<"#E gefVmeCreateMasterWindow() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
  status = gefVmeMapMasterWindow( mst_hdl, 0, V895_MAP_SIZE, &map_hdl, &ptr );
  if( status != GEF_STATUS_SUCCESS ){
    std::cout<<"#E gefVmeMapMasterWindow() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
  int d16 = 0x2;
  for(int j=0;j<16;j++) v895.vth[j] = (GEF_UINT16*)ptr +j;
  v895.width1  = (GEF_UINT16*)ptr +0x40/d16;
  v895.width2  = (GEF_UINT16*)ptr +0x42/d16;
  v895.majth   = (GEF_UINT16*)ptr +0x48/d16;
  v895.enable  = (GEF_UINT16*)ptr +0x4A/d16;
}

void vme_close()
{
  int status = 0;

  status = gefVmeUnmapMasterWindow( map_hdl );
  if(status!=GEF_STATUS_SUCCESS){
    std::cout<<"#E gefVmeUnmapMasterWindow() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
  status = gefVmeReleaseMasterWindow( mst_hdl );
  if(status!=GEF_STATUS_SUCCESS){
    std::cout<<"#E gefVmeReleaseMasterWindow() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
  status = gefVmeClose(bus_hdl);
  if(status!=GEF_STATUS_SUCCESS){
    std::cout<<"#E gefVmeClose() failed, status: "<<status<<std::endl;
    std::exit(-1);
  }
}
