#ifndef TRANSPORTMASTER_H_INCLUDED
#define TRANSPORTMASTER_H_INCLUDED

#include "mdnp3/DataLinkMaster.h"
#include "mdnp3/TransportHeader.h"
#include "mdnp3/FragmentDataEventArg.h"


namespace Dnp3Master
{

namespace TPOpStatusCode
{
enum TPOpStatusCode {Idle, Assembly};
}

class TransportMaster
{
public:
    TransportMaster();
    virtual ~TransportMaster();

    // Forward fragment data to application layer
    Dnp3FragmentDataEvent* fragment_received_event;

    void connect_to_datalink(DataLinkMaster* dlink);

    // Send userdata from application layer to datalink layer
    bool send_userdata_request(byte us[], int length, bool confirmed);

    void set_debug(bool debug_value)
    {
        _debug = debug_value;
    }

    void set_debug_stream(std::stringstream& debug_stream)
    {
        _debug_stream = &debug_stream;
    }

private:
    DataLinkMaster* _datalink_loader;
    TPOpStatusCode::TPOpStatusCode _oper_status;
    int _last_seg_index; // used when compare octet-for-octet

    /*
    Contain separate received segment from application layer
    After that, if it's valid, send to datalink layer
    */
    byte _send_segment[MAXSEGMENT];
    int _send_segment_len;

    ByteVector _rcv_fragment; // contain full valid segments received from datalink layer

    TransportHeader _send_header;
    TransportHeader _rcv_header;

    int _rcv_expected_seq;
    int _rcv_last_seq;

    bool _debug;
    std::stringstream* _debug_stream;

    void proc_segment_idlestate(byte buff[], int length);
    void proc_segment_assemblystate(byte buff[], int length);

    void notify_frame();

    /*
    Use this handler to receive segment data from datalink layer, include:
    - EventHandler object is pointed by EventHandlerBase pointer in datalink layer
    - Function to handler segment data
    */
    typedef EventHandler<TransportMaster, void, SegmentDataEventArg> Dnp3SegmentDataEventHandler;
    Dnp3SegmentDataEventHandler* _segment_data_event_handler;
    void on_segment_received(void* sender, SegmentDataEventArg* e);

};

} // Dnp3Master namespace

#endif // TRANSPORTMASTER_H_INCLUDED

