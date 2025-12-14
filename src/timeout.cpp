#include <shield/timeout.hpp>

namespace shield
{
timeout_executor::timeout_executor() 
    //: work(ioContext)
{
    thread = std::thread([this]()
    {
        ioContext.run();
    });
}

timeout_executor::~timeout_executor()
{
    ioContext.stop();
    if (thread.joinable())
    {
        thread.join();
    }
}

}