#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <cinder/Json.h>
#include <cinder/gl/gl.h>
#include <cinder/gl/Vbo.h>
#include <cinder/gl/GlslProg.h>

namespace ttrk {

  /**
  * @class Node
  * @brief An single node in the tree representation of an articulated model
  * Each element in the tree model maintains a list of 'child' nodes it's connected to and can compute transformations to its
  * coordinate systems.
  */
  class Node {

  public:
    
    /**
    * @typedef Get a pointer to this type.
    */
    typedef boost::shared_ptr<Node> Ptr;

    /**
    * @typedef Get a const pointer to this type.
    */
    typedef boost::shared_ptr<const Node> ConstPtr;

    /**
    * Default constructor. Sets the drawing flag to true.
    */
    Node() : drawing_flag_(true) {}

    /**
    * Default destructor.
    */
    virtual ~Node();

    /**
    * Pure virtual function for loading the data. Overwritten in the implementations of Node to handle different loading types.
    * @param[in] tree The JSON tree that we will parse to get the files and the children.
    * @param[in] parent The parent of this node.
    * @param[in] root_dir The root directory where the files are stored.
    * @param[in] idx The index of this node, useful for accessing nodes.
    */
    virtual void LoadData(ci::JsonTree &tree, Node *parent, const std::string &root_dir, size_t &idx) = 0;
    
    /**
    * Get the transform between the world coordinate system and this node.
    * @param[in] base_frame_pose The transform from World Coordinates to the base frame of the object.
    * @return The world transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetWorldTransform(const ci::Matrix44f &base_frame_pose) const = 0;
    
    /**
    * Get the transform between the this node and the root node.
    * @return The transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetRelativeTransformToRoot() const = 0;
    
    /**
    * Get the transform between the this node and one of its child nodes.
    * @param[in] child_idx The index of the child node.
    * @return The transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetRelativeTransformToChild(const int child_idx) const = 0;

    /**
    * Add a child node to this node.
    * @param[in] child The child node to add.
    */
    void AddChild(Node::Ptr child) { children_.push_back(child); }

    /**
    * Render a node element, will recursively call Render on all child nodes.
    * @param[in] bind_texture This flag instructs the node to bind its texture (if it has one) to the default texture target before drawing.
    */
    void Render(bool bind_texture = false);

    /**
    * Compute the jacobian for a 3D point (passed in world coordinates) with respect to this coordinate frame.
    * This will recursively call the parent frames right up to the base frame and return a 3-vector for each.
    * @param[in] point The 3D point in the target coordinate frame (usually camera).
    * @param[in] target_frame_idx The index (using the flat indexing system) of the joint which the 3D point belongs to.
    * @param[out] jacobian The jacobian which is added to by each of the joints. This needs to be a reference pass as it's filled in base to tip (roughly).
    */
    virtual void ComputeJacobianForPoint(const ci::Vec3f &point, const int target_frame_idx, std::vector<ci::Vec3f> &jacobian) const;

    /**
    * Update the pose of the model using the jacobians (with whatever cost function modification).
    * @param[in] updates The update vector iterator, there should be N for the rigid base part of the model (probably 6-7) and then one for each component of the articulated components (if there are any). The order is supposed to be the same as the order they came out from ComputeJacobians.
    */
    virtual void UpdatePose(std::vector<float>::iterator &updates) = 0;

    Node *GetParent() { return parent_; }
    
    std::vector< Node::Ptr > GetChildren() { return children_; }
    
    Node::Ptr GetChildByIdx(const std::size_t target_idx);

    Node::ConstPtr GetChildByIdx(const std::size_t target_idx) const;

    virtual ci::Matrix44f GetTransformBetweenNodes(const Node *from, const Node *to) const = 0;

    void SetDraw(const bool draw_on) { drawing_flag_ = draw_on; }

    bool HasMesh() const { return model_.getNumVertices() > 0; }

    size_t GetIdx() const { return idx_; }

  protected:

    virtual ci::Vec3f GetAxis() const = 0;

    /**
    * Load the mesh and texture from the JSON file.
    * @param[in] tree The JSON tree that we will parse to get the files and the children.
    * @param[in] root_dir The root directory where the files are stored.
    */
    void LoadMeshAndTexture(ci::JsonTree &tree, const std::string &root_dir);

    Node *parent_; /**< This node's parent. We can use a raw pointer here at it has no ownership. */
    std::vector< Node::Ptr > children_; /**< This node's children. */

    ci::TriMesh model_; /**< The 3D mesh that the model represents. */
    ci::gl::VboMesh	vbo_; /**< VBO to store the model for faster drawing. */
    ci::gl::Texture texture_; /**< The texture for the model. */

    bool drawing_flag_; /**< Switch on to enable drawing of this component. */

    size_t idx_; /**< Each node has an index for finding it in the tree. */

  };

  /**
  * @class DHNode
  * @brief A specialization of the node type to incorporate nodes which are connected by DH specified transforms.
  * A specialization of the node type to incorporate nodes which are connected by DH specified transforms rather than SE3/SO3.
  */
  class DHNode : public Node {

  public:

    /**
    * @typedef Get a pointer to this type.
    */
    typedef boost::shared_ptr<DHNode> Ptr;

    /**
    * Specialization of the load function to handle JSON files which specify DH parameters.
    * @param[in] tree The JSON tree that we will parse to get the files and the children.
    * @param[in] parent The parent of this node.
    * @param[in] root_dir The root directory where the files are stored.
    * @param[in] idx The index of this node (useful for accessing nodes).
    */
    virtual void LoadData(ci::JsonTree &tree, Node *parent, const std::string &root_dir, size_t &idx);

    /**
    * Get the transform between the world coordinate system and this node using the DH transform.
    * @param[in] base_frame_pose The transform from World Coordinates to the base frame of the object.
    * @return The world transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetWorldTransform(const ci::Matrix44f &base_frame_pose) const;

    /**
    * Get the transform between the this node and the parent node using the DH transforms.
    * @return The transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetRelativeTransformToRoot() const;

    /**
    * Get the transform between the this node and one of its child nodes using the DH transforms.
    * @return The transform in a 4x4 float matrix.
    */
    virtual ci::Matrix44f GetRelativeTransformToChild(const int child_idx) const;

    /**
    * Update the pose of the model using the jacobians (with whatever cost function modification).
    * @param[in] updates The update vector, there should be N for the rigid base part of the model (probably 6-7) and then one for each component of the articulated components (if there are any). The order is supposed to be the same as the order they came out from ComputeJacobians.
    */
    virtual void UpdatePose(std::vector<float>::iterator &updates);

    /**
    * Get the transform from the first coordinate system to the next one
    *
    */
    virtual ci::Matrix44f GetTransformBetweenNodes(const Node *from, const Node *to) const;


  protected:

    virtual ci::Vec3f GetAxis() const { return ci::Vec3f::zAxis(); }

    /**
    * Compute the rigid transform from from the parent of this coordinate system to this one using the DH parameters.
    * @return The 4x4 transform.
    */
    ci::Matrix44f ComputeDHTransform() const;

    void createFixedTransform(const ci::Vec3f &axis, const float rads, ci::Matrix44f &output) const;

    enum JointType { Rotation, Translation, Fixed, Alternative };

    JointType type_; /**< The joint type of the DH element. Dictates whether the update is applied to d or @theta. */
    float alpha_; /**< @alpha in the DH parameter set. Angle about the common normal between links. */
    float theta_; /**< @theta in the DH parameter set. Angle about the previous joint axis. */
    float a_; /**< a in the DH parameter set. Length of the common normal. */
    float d_; /**< d in the DH parameter set. Offset along previous joint axis to the common normal. */
    float update_; /**< The update to the DH set to compute the transformation. */

    ci::Vec3f alt_axis_;

  };

}


#endif