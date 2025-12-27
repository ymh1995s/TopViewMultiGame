#pragma once
class Job
{
public:
    using CallbackType = std::function<void()>;

    Job(CallbackType callback)
        : _callback(std::move(callback))
    {
    }

    void Execute()
    {
        if (_callback) _callback();
    }
public:

private:
    CallbackType _callback;
};