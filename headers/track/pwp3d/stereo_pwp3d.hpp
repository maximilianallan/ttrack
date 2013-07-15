#ifndef __STEREO_PWP3D_HPP__
#define __STEREO_PWP3D_HPP__

#include "pwp3d.hpp"

namespace ttrk {

  class StereoPWP3D : public PWP3D {
    
  public: 
    
    virtual Pose TrackTargetInFrame(KalmanTracker model, boost::shared_ptr<sv::Frame> frame);
    boost::shared_ptr<StereoCamera> &Camera() { return camera_; }
  
  protected:

     /**
    * Compute the first part of the derivative, getting a weight for each contribution based on the region agreements.
    * @param[in] r The row index of the current pixel.
    * @param[in] c The column index of the current pixel.
    * @param[in] sdf The signed distance function image.
    * @param[in] norm_foreground The normalization constant for the foreground class.
    * @param[in] norm_background The normalization constnat for the background class.
    */
    double GetRegionAgreement(const int r, const int c, const float sdf, const double norm_foreground, const double norm_background) const;

    /**
    * Get the second part of the derivative. The derivative of the contour w.r.t the pose parameters.
    * @param[in] r The row index of the current pixel.
    * @param[in] c The column index of the current pixel.
    * @param[in] sdf The value of the signed distance function for (r,c).
    * @param[in] dSDFdx The derivative of the signed distance function /f$\frac{\partial SDF}{\partial x}/f$ at the current pixel.
    * @param[in] dSDFdy The derivative of the signed distance function /f$\frac{\partial SDF}{\partial y}/f$ at the current pixel.
    * @return The pose derivitives as a vector.
    */
    cv::Mat GetPoseDerivatives(const int r, const int c, const cv::Mat &sdf, const float dSDFdx, const float dSDFdy, KalmanTracker &current_model);
    
    /**
    * Construct a signed distance function of the outer contour of the shape projected into the image plane.
    * @param[in] current_model The model which will be projected to the image plane.
    * @return The image containin the signed distance function. Will be a single channel floating point image.
    */
    const cv::Mat ProjectShapeToSDF(KalmanTracker &current_model);

    /**
    * Computes the normalization constant of the pose update equation.
    * @param[out] norm_foreground The normalization constant for the foreground class.
    * @param[out] norm_background The normalization constant for the background class.
    * @param[in] sdf_image The signed distance function image.
    */
    void ComputeNormalization(double &norm_foreground, double &norm_background, const cv::Mat &sdf_image) const;

      /**
    * Finds an intersection between a ray cast from the current pixel through the tracked object.
    * @param[in] r The row index of the pixel.
    * @prarm[in] c The column index of the pixel.
    * @param[out] front_intersection The intersection between the ray and the front of the object.
    * @param[out] back_intersection The intersection between the ray and the back of the object.
    * @return bool The success of the intersection test.
    */
    bool GetTargetIntersections(const int r, const int c, cv::Vec3f &front_intersection, cv::Vec3f &back_intersection, const KalmanTracker &current_model) const ;
    
    bool GetNearestIntersection(const int r, const int c, const cv::Mat &sdf, cv::Vec3f &front_intersection, cv::Vec3f &back_intersection, const KalmanTracker &current_model) const ;
    
    cv::Mat GetRegularizedDepth(const int r, const int c, const KalmanTracker &kalman_tracker) const;

    inline cv::Vec3f GetDOFDerivatives(const int dof, const Pose &pose, const cv::Vec3f &point) const ;
    virtual void FindROI(const std::vector<cv::Vec2i> &convex_hull);

    void DrawModelOnFrame(const KalmanTracker &tracked_model, cv::Mat canvas) const;

    boost::shared_ptr<StereoCamera> camera_;
    cv::Mat ROI_left_; /**< Experimental feature. Instead of performing the level set tracking over the whole image, try to find a ROI around where the target of interest is located. */
    cv::Mat ROI_right_; /**< Experimental feature. Instead of performing the level set tracking over the whole image, try to find a ROI around where the target of interest is located. */

  };

  cv::Vec3f StereoPWP3D::GetDOFDerivatives(const int dof, const Pose &pose, const cv::Vec3f &point_) const {

    //derivatives use the (x,y,z) from the initial reference frame not the transformed one so inverse the transformation
    cv::Vec3f point = point_ - pose.translation_;
    point = pose.rotation_.Inverse().RotateVector(point);

    //return the (dx/dL,dy/dL,dz/dL) derivative for the degree of freedom
    switch(dof){
    
    case 0: //x
      return cv::Vec3f(1,0,0);
    case 1: //y
      return cv::Vec3f(0,1,0);
    case 2: //z
      return cv::Vec3f(0,0,1);
    
    
    case 3: //qw
      return cv::Vec3f((2*pose.rotation_.Y()*point[2])-(2*pose.rotation_.Z()*point[1]),
                       (2*pose.rotation_.Z()*point[0])-(2*pose.rotation_.X()*point[2]),
                       (2*pose.rotation_.X()*point[1])-(2*pose.rotation_.Y()*point[0]));

    case 4: //qx
      return cv::Vec3f((2*pose.rotation_.Y()*point[1])+(2*pose.rotation_.Z()*point[2]),
                       (2*pose.rotation_.Y()*point[0])-(4*pose.rotation_.X()*point[1])-(2*pose.rotation_.W()*point[2]),
                       (2*pose.rotation_.Z()*point[0])+(2*pose.rotation_.W()*point[1])-(4*pose.rotation_.X()*point[2]));

    case 5: //qy
      return cv::Vec3f((2*pose.rotation_.X()*point[1])-(4*pose.rotation_.Y()*point[0])+(2*pose.rotation_.W()*point[2]),
                       (2*pose.rotation_.X()*point[0])+(2*pose.rotation_.Z()*point[2]),
                       (2*pose.rotation_.Z()*point[1])+(2*pose.rotation_.W()*point[0])-(4*pose.rotation_.Y()*point[2]));
    case 6: //qz
      return cv::Vec3f((2*pose.rotation_.X()*point[2])-(2*pose.rotation_.W()*point[1])-(4*pose.rotation_.Z()*point[0]),
                       (2*pose.rotation_.W()*point[0])-(4*pose.rotation_.X()*point[1])+(2*pose.rotation_.Y()*point[2]),
                       (2*pose.rotation_.X()*point[0])+(2*pose.rotation_.Y()*point[1]));

    default:
      throw std::runtime_error("Error, a value in the range 0-6 must be supplied");
    }
  }
  



}

#endif
