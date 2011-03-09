#ifndef IDMLIB_SSP_MATRIXFILEIO_H_
#define IDMLIB_SSP_MATRIXFILEIO_H_


#include <string>
#include <iostream>
#include <idmlib/idm_types.h>
#include <am/sequence_file/SequenceFile.hpp>

NS_IDMLIB_SSP_BEGIN

template <typename VT, typename I = uint32_t>
class MatrixFileFLIo
{
public:

  MatrixFileFLIo(const std::string& dir):dir_(dir), storage_(NULL)
  {
    
  }
  ~MatrixFileFLIo()
  {
    if(storage_!=NULL)
    {
      delete storage_;
    }
  }
  
  bool Open()
  {
    try
    {
      boost::filesystem::create_directories(dir_);
      std::string storage_file = dir_+"/storage";
      storage_ = new izenelib::am::SequenceFile<VT>(storage_file);
      storage_->open();
    }
    catch(std::exception& ex)
    {
      std::cout<<ex.what()<<std::endl;
      return false;
    }
    return true;
  }
  
  bool Flush()
  {
    storage_->flush();
    return true;
  }
  
  
  bool GetVector(I id, VT& vec)
  {
    return storage_->get(id, vec);
  }
  
  bool SetVector(I id, const VT& vec)
  {
    storage_->update(id, vec);
    return true;
  }
  
 
 private: 
  std::string dir_;
  izenelib::am::SequenceFile<VT>* storage_;
//   FunctionType callback_;
};

   
NS_IDMLIB_SSP_END



#endif 
