/* Copyright 2022 The LightSeq Team */
#pragma once
#include "cstdio"
#include "queue"
#include "deque"
#include "stack"

#include "declaration.h"
#include "manager.h"
#include "layer.h"
#include "node.h"
#include "cuda_util.h"
#include "unordered_map"

namespace lightseq {

enum StatusType { Training, Inference, Evaluation };
const std::string StatusTypeString[] = {"Training", "Inference", "Evaluation"};

class Context {  // model only
 private:
  static std::unordered_map<std::string, std::shared_ptr<void>> pybind_layers;
  std::unordered_map<std::string, void*> _resources_pool;

  std::vector<Node*> _all_node_vec{};
  std::vector<Operator*> _model_ops{};
  std::vector<Layer*> _root_layers{};
  std::vector<Layer*> _all_layers{};
  std::deque<Layer*> _layer_context;
  StatusType _status_type;

  bool _built = false;
  bool _building = false;
  int _node_idx = 0;
  MemoryManagerPtr _mm_ptr;

  int _device_id;

  cudaStream_t _stream;
  cublasHandle_t _cublasHandle;
  static std::shared_ptr<Context> _global_context_ptr;

  bool check_validate();

  static std::unordered_map<int, std::shared_ptr<Context>> global_contexts_map;
  static int global_context_id;

  int _regress_begin_idx = -1;
  int _regress_end_idx = -1;
  bool _in_regress = false;

 public:
  Context(StatusType status_type = StatusType::Inference, int device_id = 0);
  virtual ~Context();

  cudaStream_t get_stream() { return _stream; }

  cublasHandle_t get_cublashandle() { return _cublasHandle; }

  void set_stream(cudaStream_t stream);

  void convert_into_train();
  void convert_into_eval();

  static int create_global_context(
      StatusType status_type = StatusType::Inference, int device_id = -1);
  static void set_global_context(int context_id);
  static std::shared_ptr<Context> global_instance();

  // for initial calculation
  size_t mx_tensor_size = 0;
  char* temporary_buffer_ = nullptr;

  std::map<std::string, int> layer_name_cnt;
  std::map<std::string, int> node_name_cnt;

  // property field
  bool is_training() { return _status_type == StatusType::Training; }
  int node_idx() { return _node_idx; }
  void update_node_idx();
  bool is_built() { return _built; }
  bool is_building() { return _building; }
  MemoryManagerPtr memory_manager_ptr() { return _mm_ptr; }

  void add_op(Operator* op);
  void add_node(Node* node);

  void enter_layer(Layer* cur_layer, bool is_initial = true);

  // collaborate with enter_layer()
  void exit_layer() { _layer_context.pop_back(); }

  void build();

  void draw_all_context();

  Layer* last_layer() {
    return _layer_context.size() ? _layer_context.back() : nullptr;
  }
  Node* last_node() {
    return _all_node_vec.size() ? _all_node_vec[_all_node_vec.size() - 1]
                                : nullptr;
  }

  void regress_begin() { _in_regress = true; }
  void regress_end() { _in_regress = false; }
  int regress_begin_idx() { return _regress_begin_idx; }
  int regress_end_idx() { return _regress_end_idx; }
  void update_regr_begin(int node_idx);
  void update_regr_end(int node_idx);
  bool in_regress() { return _in_regress; }

  std::string status_type_str() { return StatusTypeString[_status_type]; }

  void register_object(std::string object_name, void* object);
  void* get_object(std::string object_name);

  static void regist_pybind_layer(std::string layer_name, int layer_id,
                                  std::shared_ptr<void> layer_ptr);
  static std::shared_ptr<void> get_pybind_layer(std::string layer_name,
                                                int layer_id);
};

#define REGISTER_OBJECT(objname) register_object_func(#objname, objname)
#define GET_OBJECT(objname) get_object_func(#objname)

}  // namespace lightseq
