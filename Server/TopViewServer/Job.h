#pragma once
class Job
{
public:
    using CallbackType = std::function<void()>;

    Job(CallbackType callback, int targetQueue)
        : _callback(std::move(callback)),
        _targetQueue(targetQueue)
    {
    }

    void Execute()
    {
        if (_callback) _callback();
    }
public:
    int _targetQueue = -1; // 담당할 큐의 번호

private:
    CallbackType _callback;
};