#include <QCoreApplication>
#include <iostream>
#include <memory>
#include <future>
#include <thread>
#include <chrono>
#include <csignal>
#include <QDebug>

#include "asynchmgrtest.h"
#include "../include/device_abstraction/material/channel.h"

using derivedType = int;

template<typename T>
struct base{
    virtual void getObject(T) =0;
};

struct derived : public base<derivedType>{
    void getObject(derivedType arg)
    {
        std::cout << "get param: " << arg << std::endl;
    }
};

struct AbstractData{
    // nothing here
    virtual ~AbstractData() { std::cout << "~AbstractData" << std::endl; }
};

struct ConcreteData : AbstractData{
    using Data_t = int;
    Data_t value;

    ~ConcreteData() { std::cout << "~ConcreteData" << std::endl; }
};

struct A{
    std::unique_ptr<AbstractData> data;

    A(decltype(data) initializer) { data.swap(initializer); }
    virtual ~A() { std::cout << "~A" << std::endl; }
    virtual std::string classInfo() const { return "this is A class"; }
};

struct B : A{
    B() : A{ std::make_unique<ConcreteData>() } { }
    ~B() { std::cout << "~B" << std::endl; }

    ConcreteData::Data_t parseConcreteData(AbstractData* dt) const {
        return ((ConcreteData*)dt)->value;
    }

    std::string classInfo() const {
        return &"this is B class and it has data member with value: "
            [ parseConcreteData(data.get())];
    }
};

struct C : A{
    std::string classInfo() const { return "this is C class"; }
};

void getObjectInfo(A* obj)
{
    std::cout << obj->classInfo() << std::endl;
}

using ptr_t = std::unique_ptr<int>;

ptr_t getPtr()
{
    auto ptr1 = std::make_unique<int>();
    auto ptr2 = std::move(ptr1);
    return ptr2;
}

class DebugClass
{
public:
    DebugClass() { std::cout << "DebugClass constructor [" << this << "]" << std::endl; }
    ~DebugClass() { std::cout << "DebugClass destructor [" << this << "]" << std::endl; }
};

auto asynchTest()
{
    std::cout << "START asynch test in thread [" << std::this_thread::get_id() << "]" << std::endl;
    auto shr = std::make_shared<DebugClass>();
    auto ftr = std::async(std::launch::async, [handle = shr](){
        std::cout << "START parallel thread [" << std::this_thread::get_id() << "]" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        std::cout << "END parallel thread" << std::endl; }).share();
    std::cout << "END asynch test" << std::endl;
    return ftr;
}

namespace {
volatile std::atomic<std::thread::id> thread_id;
}

void threadCheck(int)
{
    thread_id = std::this_thread::get_id();
}

void asynchRaiseTest()
{
    auto ftr = std::async(std::launch::async, [](){
        std::cout << "START parallel thread [" << std::this_thread::get_id() << "]" << std::endl;
        if (std::raise(SIGINT))
            std::cerr << "failed to RAISE custom signal" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "END parallel thread" << std::endl; });
}


int main(int argc, char *argv[])
{
//    if (std::signal(SIGINT, threadCheck) == SIG_ERR)
//    {
//        std::cerr << "failed to ADD custom signal" << std::endl;
//        return -1;
//    }
//    std::cout << "START signal test in thread [" << std::this_thread::get_id() << "]" << std::endl;
//    asynchRaiseTest();
//    std::cout << "signal was RAISED in thread [" << thread_id << "]" << std::endl;
//    std::cout << "END signal test in thread [" << std::this_thread::get_id() << "]" << std::endl;
//    return 0;

    QCoreApplication app{argc, argv};
    auto thrd = QThread{};
    auto tst = AsynchMgrTest{};
    tst.moveToThread(&thrd);
    tst.threads();
    QObject::connect(&thrd, &QThread::started,
                     &tst, &AsynchMgrTest::test);
    thrd.start();
    while (tst.finished == false)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    thrd.exit();
    thrd.wait();

    return 0;

//    auto ftr = asynchTest();
//    std::cout << "OUT of asynch test" << std::endl;
//    return 0;

//    auto obj = std::unique_ptr<A>(new B);
//    getObjectInfo(obj.get());

//    auto ptr{ getPtr() };

//    return 0;
}
