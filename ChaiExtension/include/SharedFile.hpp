#include <memory>
#include <string>
#include <stdexcept>
using File = std::shared_ptr<FILE>;
File SharedFile(const std::string& file, const std::string& mode)
{
  std::shared_ptr<FILE> f(
    // resource alloc
    fopen(file.c_str(),mode.c_str()),
    // deleter
    [=](FILE* f)
    {
      fclose(f);
    });

  if(f.get() == nullptr)
  {
    std::string msg = "error opening file:";
    msg += file;
    throw std::runtime_error(msg);
  }
  return f;
}
