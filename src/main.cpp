#include "../headers/ttrack.hpp"

int main(int argc, char **argv){

  ttrk::TTrack &t = ttrk::TTrack::Instance();
  
  try{
    
    t.SetUp("./data/data_11",ttrk::RF,ttrk::SEPARATE);
    t.RunImages();
    //t.TestDetector("./data/data_11/data/test_images/image_19.png","output.png");

  }catch(std::runtime_error &e){
    std::cerr << e.what() << "\n";
#if defined(_WIN32) || defined(_WIN64)
    system("pause");
#endif
  }

  ttrk::TTrack::Destroy();
#if defined(_WIN32) || defined(_WIN64)
  _CrtDumpMemoryLeaks();
#endif
  return 0;

}
