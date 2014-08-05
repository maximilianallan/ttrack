#include <boost/filesystem.hpp>

#include "../../../include/track/pwp3d/stereo_pwp3d.hpp"
#include "../../../include/utils/helpers.hpp"
#include "../../../include/utils/renderer.hpp"

using namespace ttrk;

void StereoPWP3D::SwapToLeft(Pose &pose){

  boost::shared_ptr<sv::StereoFrame> stereo_frame = boost::dynamic_pointer_cast<sv::StereoFrame>(frame_);

  camera_ = stereo_camera_->rectified_left_eye();
  //swap everything back
  stereo_frame->SwapEyes();
  Pose extrinsic;
  if(stereo_camera_->IsRectified())
    extrinsic = Pose(cv::Vec3d(stereo_camera_->ExtrinsicTranslation()[0],0,0),sv::Quaternion(cv::Mat::eye(3,3,CV_64FC1)));
  else
    extrinsic = Pose(stereo_camera_->ExtrinsicTranslation(),sv::Quaternion(stereo_camera_->ExtrinsicRotation()));
  pose = CombinePoses(extrinsic.Inverse(),pose);

}


void StereoPWP3D::SwapToRight(Pose &pose){

  boost::shared_ptr<sv::StereoFrame> stereo_frame = boost::dynamic_pointer_cast<sv::StereoFrame>(frame_);
  camera_ = stereo_camera_->rectified_right_eye();
  //swap the roi over in the images so they refer to the right hand image
  stereo_frame->SwapEyes();
  //also update the object pose so that it's relative to the right eye
  Pose extrinsic;
  if(stereo_camera_->IsRectified())
    extrinsic = Pose(cv::Vec3d(stereo_camera_->ExtrinsicTranslation()[0],0,0),sv::Quaternion(cv::Mat::eye(3,3,CV_64FC1)));
  else
    extrinsic = Pose(stereo_camera_->ExtrinsicTranslation(),sv::Quaternion(stereo_camera_->ExtrinsicRotation()));
  pose = CombinePoses(extrinsic, pose);

}

bool StereoPWP3D::SwapEye(Pose &pose){

  switch (current_eye_) {

  case LEFT:  
    return true;

  case RIGHT:
    SwapToRight(pose);
    return true;
  
  default:
    SwapToLeft(pose);
    return false;

  }

}

void StereoPWP3D::GetFastDOFDerivsLeft(const Pose &pose, double *pose_derivs, double *intersection){
  pose.GetFastDOFDerivs(pose_derivs,intersection);
}

void StereoPWP3D::GetFastDOFDerivsRight(const Pose &pose, double *pose_derivs, double *intersection){
  pose.GetFastDOFDerivs(pose_derivs,intersection);
  static Pose extrinsic;
  if(stereo_camera_->IsRectified())
    extrinsic = Pose(cv::Vec3d(stereo_camera_->ExtrinsicTranslation()[0],0,0),sv::Quaternion(cv::Mat::eye(3,3,CV_64FC1)));
  else
    extrinsic = Pose(stereo_camera_->ExtrinsicTranslation(),sv::Quaternion(stereo_camera_->ExtrinsicRotation()));

  cv::Vec3d point_in_left_coords = extrinsic.InverseTransform(cv::Vec3d(intersection[0],intersection[1],intersection[2])); //point in left camera coordinates

  CombinePoses(extrinsic.Inverse(),pose).GetFastDOFDerivs(pose_derivs,point_in_left_coords.val); //get derivatives from the perspective of the left camera

  //rotate vector and copy it back
  for(int i=0;i<21;i+=3){
    cv::Vec3d x = extrinsic.rotation_.RotateVector( cv::Vec3d(pose_derivs[i],pose_derivs[i+1],pose_derivs[i+2]) );
    memcpy(pose_derivs+i,x.val,3*sizeof(double));
  }
}



void StereoPWP3D::GetFastDOFDerivs(const Pose &pose, double *pose_derivs, double *intersection) {

  switch( current_eye_ ){

  case LEFT: //derivatives are super simple for the left eye
    GetFastDOFDerivsLeft(pose,pose_derivs,intersection);
    break;

  case RIGHT: //more complex for the right eye
    GetFastDOFDerivsRight(pose,pose_derivs,intersection);
    break;

  default:
    throw(std::runtime_error("Error, something is very wrong...!\n"));
  
  }

}

/*
struct CostFunctor {
  template <typename T>
  bool operator()(const T* const x, T* residual) const {
    residual[0] = T(10.0) - x[0];
    return true;
  }
};
*/
Pose StereoPWP3D::TrackTargetInFrame(KalmanTracker current_model, boost::shared_ptr<sv::Frame> frame){
 
  //using namespace ceres;
  frame_ = frame;
  
  cv::Mat sdf_image__, front_intersection_image__, back_intersection_image__;
  return current_model.CurrentPose();
  

  boost::shared_ptr<sv::StereoFrame> stereo_frame = boost::dynamic_pointer_cast<sv::StereoFrame>(frame);
  SetBlurringScaleFactor(stereo_frame->GetLeftImage().cols);
  const int NUM_STEPS = 15;
  cv::Vec3d initial_translation = current_model.CurrentPose().translation_;


  /*cv::Mat sdf_image,front_intersection_image,back_intersection_image;
  GetSDFAndIntersectionImage(current_model,sdf_image,front_intersection_image,back_intersection_image);
  register_points_.ComputeDescriptorsForPointTracking(frame_,current_model,sdf_image);
  SAFE_EXIT();*/

  // Build the problem.
  //ceres::Problem problem;
  // The variable to solve for with its initial value.
  double initial_x = 5.0;
  double x = initial_x;

  // Set up the only cost function (also known as residual). This uses
  // auto-differentiation to obtain the derivative (jacobian).
  //ceres::CostFunction* cost_function =
  //  new ceres::AutoDiffCostFunction<CostFunctor, 1, 1>(new CostFunctor);
  //problem.AddResidualBlock(cost_function, NULL, &x);

  // Run the solver!
  //Solver::Options options;
  //options.linear_solver_type = ceres::DENSE_QR;
  //options.minimizer_progress_to_stdout = true;
  //Solver::Summary summary;
  //Solve(options, &problem, &summary);

  //iterate until convergence
  for(int step=0,pixel_count=0; step < NUM_STEPS; step++,pixel_count=0){

    //(x,y,z,w,r1,r2,r3)
    PoseDerivs image_pose_derivatives = PoseDerivs::Zeros();
    cv::Mat left_canvas, right_canvas, left_z_buffer, right_z_buffer, left_binary_image, right_binary_image;
    GetRenderedModelAtPose(current_model, left_canvas, left_z_buffer, left_binary_image, right_canvas, right_z_buffer, right_binary_image);

    for(current_eye_ = LEFT; ;current_eye_++){ //iterate over the eyes

      if(!SwapEye(current_model.CurrentPose())) break; //sets the camera_ variable to left or right eye breaks after resetting everything back to initial state after right ey
        
      cv::Mat sdf_image,front_intersection_image,back_intersection_image;
      
      if (current_eye_ == LEFT)
        ProcessSDFAndIntersectionImage(current_model, left_z_buffer, sdf_image, left_binary_image, front_intersection_image, back_intersection_image);
      else if (current_eye_ == RIGHT)
        ProcessSDFAndIntersectionImage(current_model, right_z_buffer, sdf_image, right_binary_image, front_intersection_image, back_intersection_image);

      //compute the derivates of the sdf images
      cv::Mat dSDFdx, dSDFdy;

      //cv::Sobel(sdf_image,dSDFdx,CV_32FC1,1,0,1); // (src,dst,dtype,dx,dy,size) size = 1 ==> 3x1 finite difference kernel
      //cv::Sobel(sdf_image,dSDFdy,CV_32FC1,0,1,1);
      cv::Scharr(sdf_image,dSDFdx,CV_32FC1,1,0);
      cv::Scharr(sdf_image,dSDFdy,CV_32FC1,0,1);

      for(int r=0;r<frame_->GetImageROI().rows;r+=3){
        for(int c=0;c<frame_->GetImageROI().cols;c+=3,++pixel_count){

          //speedup tests by checking if we need to evaluate the cost function in this region
          const double skip = Heaviside(sdf_image.at<double>(r,c), k_heaviside_width_ * blurring_scale_factor_);
          if( skip < 0.00001 || skip > 0.99999 ) continue;

          //P_f - P_b / (H * P_f + (1 - H) * P_b)
          const double region_agreement = GetRegionAgreement(r, c, sdf_image.at<double>(r,c));

          //dH / dL
          PoseDerivs per_pixel_pose_derivatives;
          GetPoseDerivatives(r, c, sdf_image, dSDFdx.at<double>(r,c), dSDFdy.at<double>(r,c), current_model, front_intersection_image, back_intersection_image, per_pixel_pose_derivatives);
          per_pixel_pose_derivatives.MultiplyByValue(-1 * region_agreement);

        }
      }

    }

    std::vector<MatchedPair> pnp_pairs;
    cv::Mat point_save_image = frame_->GetImageROI().clone();
    register_points_.FindPointCorrespondencesWithPose(frame_,current_model.PtrToModel(),current_model.CurrentPose(),point_save_image);
    for(auto pnp=pnp_pairs.begin();pnp!=pnp_pairs.end();pnp++){
      register_points_.GetPointDerivative(pnp->learned_point,cv::Point2d(pnp->image_point[0],pnp->image_point[1]), current_model.CurrentPose(), image_pose_derivatives);
    } 
    
    //scale jacobian and update pose
    ApplyGradientDescentStep(image_pose_derivatives,current_model.CurrentPose(),step,pixel_count);
  
  }

  //update the velocity model... a bit crude
  cv::Vec3d translational_velocity = current_model.CurrentPose().translation_ - initial_translation;
  current_model.CurrentPose().translational_velocity_ = translational_velocity;

  return current_model.CurrentPose();

}

void StereoPWP3D::GetRenderedModelAtPose(const KalmanTracker &current_model, cv::Mat &left_canvas, cv::Mat &left_z_buffer, cv::Mat &left_binary_image, cv::Mat &right_canvas, cv::Mat &right_z_buffer, cv::Mat &right_binary_image) const {

  Renderer::DrawStereoMesh(current_model.PtrToModel(), left_canvas, left_z_buffer, left_binary_image, current_model.CurrentPose(), camera_);

}

