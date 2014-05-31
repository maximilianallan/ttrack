#include "../include/ttrack_app.hpp"
#include "cinder/app/AppNative.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/Rand.h"
#include "cinder/TriMesh.h"
#include "../include/resources.hpp"
#include "cinder/ObjLoader.h"
#include "cinder/Json.h"
#include <vector>
#include <utility>
#include <boost/ref.hpp>
#include "CinderOpenCV.h"
#include <cinder/gl/Fbo.h>
using namespace ci;
using namespace ci::app;


void TTrackApp::setup(){

  const std::string root_dir = "../data/lnd";

  auto &ttrack = ttrk::TTrack::Instance();
  ttrack.SetUp(root_dir + "/" + "model/model.json", root_dir + "/" + "camera/config.xml", root_dir + "/" + "classifier/config.xml", root_dir + "/" + "results/", ttrk::RF, ttrk::STEREO, root_dir + "/left.avi", root_dir + "/right.avi");
  time_ = getElapsedSeconds();
 
  shader_ = gl::GlslProg( loadResource( RES_SHADER_VERT ), loadResource( RES_SHADER_FRAG ) );
 
  setWindowSize(736 * 2, 288 * 2);

  //CameraStereo cam;
  CameraPersp cam;
  cam.setEyePoint( Vec3f(0.0f,0.0f,0.0f) );
 //cam.setCenterOfInterestPoint( Vec3f(0.0f, 0.0f, -1.0f) ); //look down -z
  cam.setViewDirection(ci::Vec3f(0, 0, -1));
  cam.setPerspective( 70.0f, getWindowAspectRatio(), 1.0f, 1000.0f );
  maya_cam_.setCurrentCam( cam );
  
  gl::Fbo::Format msaaFormat;
  msaaFormat.setSamples(4); // enable 4x MSAA

  framebuffer_.reset(new gl::Fbo(getWindowWidth(), getWindowHeight()));// , msaaFormat));
  boost::thread main_thread(boost::ref(ttrack));
 
}

void TTrackApp::update(){

  double elapsed = getElapsedSeconds() - time_;
  time_ = getElapsedSeconds();

  returnRenderable(); //check to see if the renderer has processed any frames

  auto &ttrack = ttrk::TTrack::Instance();
  if (!ttrack.GetLatestUpdate(irs_)){
    return;
  }
  
  ci::ImageSourceRef img = ci::fromOcv(irs_->first->GetImageROI());
  frame_texture_ = gl::Texture(img);

}

bool TTrackApp::returnRenderable(){

  ttrk::WriteLock w_lock(ttrk::Renderer::mutex);

  if (to_render_ == nullptr)
    return false;

  ttrk::Renderer &r = ttrk::Renderer::Instance();
 
  to_render_->canvas_ = toOcv(framebuffer_->getTexture());
  to_render_->z_buffer_ = toOcv(framebuffer_->getDepthTexture());
  
  r.rendered = std::move(to_render_); //give it back
  
  return true;

}


void TTrackApp::drawRenderable(boost::shared_ptr<ttrk::Model> mesh, const ttrk::Pose &pose){

  framebuffer_->bindFramebuffer();

  gl::clear(ci::Color(0,0,0));
  
  auto meshes_textures_transforms = mesh->GetRenderableMeshes();
  
  const ci::Matrix44d current_pose = pose.AsCiMatrixForOpenGL();

  for (auto mesh_tex_trans = meshes_textures_transforms.begin(); mesh_tex_trans != meshes_textures_transforms.end(); ++mesh_tex_trans){

    auto texture = mesh_tex_trans->get<1>();

    gl::pushModelView();
    gl::multModelView(current_pose * mesh_tex_trans->get<2>());

    const auto trimesh = mesh_tex_trans->get<0>();
    gl::draw(*trimesh);

    gl::popModelView();
  }

  framebuffer_->unbindFramebuffer();

}

void TTrackApp::checkRenderer(){

  ttrk::WriteLock w_lock(ttrk::Renderer::mutex);
  
  ttrk::Renderer &r = ttrk::Renderer::Instance();

  if (r.to_render.get() != nullptr && r.rendered.get() == nullptr){
    to_render_ = std::move(r.to_render); //take ownership
    drawRenderable(to_render_->mesh_,to_render_->pose_); //make the draw command 
  }

}

void TTrackApp::draw3D() {

  if (!irs_) {
    return;
  }

  //only draw the meshes if we actually have some to draw
  drawMeshes();
  
}

void TTrackApp::draw2D() {
      
  if( frame_texture_ )
    gl::draw(frame_texture_, getWindowBounds() );
  
}

void TTrackApp::drawMeshes() {

  for(auto model = irs_->second.begin(); model != irs_->second.end(); ++model)  {
    auto meshes_textures_transforms = model->PtrToModel()->GetRenderableMeshes();
    ci::Matrix44d current_pose = model->CurrentPose().AsCiMatrixForOpenGL();

    for(auto mesh_tex_trans = meshes_textures_transforms.begin();mesh_tex_trans != meshes_textures_transforms.end();++mesh_tex_trans){
      
      auto texture = mesh_tex_trans->get<1>();
      
      gl::pushModelView();
      gl::multModelView(current_pose * mesh_tex_trans->get<2>());
      
      const auto trimesh = mesh_tex_trans->get<0>();
      gl::draw( *trimesh );
      
      gl::popModelView();

    }
  }

}


void TTrackApp::draw(){
  
  gl::pushMatrices();
  gl::setMatrices(maya_cam_.getCamera());

  gl::enableDepthRead();
  gl::enableDepthWrite();

  gl::enableAlphaBlending();

  shader_.bind();

  checkRenderer();

  gl::clear( Color( 0.0f, 0.0f, 0.0f ) , true ); //set the screen to black and clear the depth buffer
  
  //draw2D();

	draw3D();

  shader_.unbind();
  gl::disableAlphaBlending();

  gl::popMatrices();
}


void TTrackApp::keyDown( KeyEvent event ){

  if(event.getChar() == ' '){

    quit();

  } 
  
}

void TTrackApp::mouseMove( MouseEvent event ){
  // keep track of the mouse
  mouse_pos_ = event.getPos();
}

void TTrackApp::mouseDown( MouseEvent event ){	
  // let the camera handle the interaction
  maya_cam_.mouseDown( event.getPos() );
}

void TTrackApp::mouseDrag( MouseEvent event ){
  // keep track of the mouse
  mouse_pos_ = event.getPos();

  // let the camera handle the interaction
  maya_cam_.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void TTrackApp::resize(){
  CameraPersp cam = maya_cam_.getCamera();
  cam.setAspectRatio( getWindowAspectRatio() );
  maya_cam_.setCurrentCam( cam );
}



CINDER_APP_NATIVE( TTrackApp, RendererGl )
