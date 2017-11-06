#ifndef EVENTHANDLER_H_INCLUDED
#define EVENTHANDLER_H_INCLUDED

/*
Xay dung dua tren tinh chat ke thua va da hinh cua lop
Moi quan he giua nguon phat va nguon thu la 1-1

Tai nguon phat:
- Chua con tro p1 loai EventHandlerBase
Tai nguon thu:
- Tao instance obj1 cua EventHandler
- Tao function don nhan du lieu tu nguon phat co dang function_name(SenderT* sender, EventArgT* e)
- Dang ky function_name vao doi tuong obj1
- Cho con tro p1 tro toi obj1

Khi nguon phat co su kien thi p1 se goi notify cua obj1 vi:
- EventHandler ke thua EvenHandlerbase
- notify duoc override boi EventHandler
- p1 dang tro toi obj1
*/
template<class SenderT, class EventArgT>
class EventHandlerBase
{
public:
    EventHandlerBase() {};
    virtual ~EventHandlerBase() {};

    virtual void notify(SenderT* sender, EventArgT* e) = 0;
};

template<class ListenerT, class SenderT, class EventArgT>
class EventHandler : public EventHandlerBase<SenderT, EventArgT>
{
    typedef void(ListenerT::*pfunc_callback)(SenderT*, EventArgT*);
public:
    EventHandler()
    {
        _listener = NULL;
        _callback = NULL;
    }
    EventHandler(ListenerT* listener, pfunc_callback callback)
    {
        _listener = listener;
        _callback = callback;
    }

    virtual ~EventHandler() {}

    void connect(ListenerT* listener, pfunc_callback callback)
    {
        if (listener)
        {
            _listener = listener;
            _callback = callback;
        }
    }

    void disconnect()
    {
        _listener = NULL;
        _callback = NULL;
    }

    virtual void notify(SenderT* sender, EventArgT* e) //override
    {
        if (_listener) (_listener->*_callback)(sender, e);
    }

private:
    ListenerT* _listener;
    pfunc_callback _callback;
};

#endif // EVENTHANDLER_H_INCLUDED

