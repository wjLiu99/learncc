#include "arpc_controller.h"

arpc_controller::arpc_controller()
{
    failed_ = false;
    err_text_ = "";
}

void arpc_controller::Reset()
{
    failed_ = false;
    err_text_ = "";
}

bool arpc_controller::Failed() const
{
    return failed_;
}

std::string arpc_controller::ErrorText() const
{
    return err_text_;
}

void arpc_controller::SetFailed(const std::string& reason)
{
    failed_ = true;
    err_text_ = reason;
}


void arpc_controller::StartCancel(){}
bool arpc_controller::IsCanceled() const {return false;}
void arpc_controller::NotifyOnCancel(google::protobuf::Closure* callback) {}