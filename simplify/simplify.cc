#include "simplify.hh"

#include <v8.h>
#include <libplatform/libplatform.h>

static v8::Platform *g_platform;
namespace simplify {

void Initialize()
{
  using namespace v8;

  //V8::InitializeICU();
  g_platform = v8::platform::CreateDefaultPlatform();
  V8::InitializePlatform(g_platform);
  V8::Initialize();
}

void TearDown()
{
  using namespace v8;
}

}  // namespace simplify
