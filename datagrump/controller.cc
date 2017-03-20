#include <iostream>
#include <cassert>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

const uint64_t MAX_REORDER_MS = 5;

/* Default constructor */
Controller::Controller( const double up_delay_threshold, const double up_delay_window_delta, const double down_delay_threshold, const double down_delay_window_delta, const double loss_window_delta )
  : up_delay_threshold_( up_delay_threshold )
  , up_delay_window_delta_( up_delay_window_delta )
  , down_delay_threshold_( down_delay_threshold )
  , down_delay_window_delta_( down_delay_window_delta )
  , loss_window_delta_( loss_window_delta )
  , the_window_size(16)
  , skewed_lowest_owt(99999)
  , lowest_rtt(99999)
  , datagram_list_()
{
    assert( up_delay_threshold <= down_delay_threshold );
}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  datagram_list_.emplace_back(sequence_number, send_timestamp);
}

/* An ack was received */
void Controller::ack_received( const uint64_t sequence_number_acked,
			       /* what sequence number was acknowledged */
			       const uint64_t send_timestamp_acked,
			       /* when the acknowledged datagram was sent (sender's clock) */
			       const uint64_t recv_timestamp_acked,
			       /* when the acknowledged datagram was received (receiver's clock)*/
			       const uint64_t timestamp_ack_received )
                               /* when the ack was received (by sender) */
{
    while ( datagram_list_.front().second + MAX_REORDER_MS < send_timestamp_acked ) {
        // packet assumed lost
        datagram_list_.pop_front();
        the_window_size -= loss_window_delta_;
    }

    auto it = datagram_list_.begin();
    while ( it != datagram_list_.end() ) {
        if ( it->first == sequence_number_acked ) {
            it = datagram_list_.erase( it );
            break;
        } else if ( it->first > sequence_number_acked ) {
            break;
        } else {
            it++;
        }
    }

    int64_t skewed_owt = recv_timestamp_acked - send_timestamp_acked;
    if ( skewed_owt < skewed_lowest_owt ) {
        skewed_lowest_owt = skewed_owt;
    }

    double rtt =  timestamp_ack_received - send_timestamp_acked;
    lowest_rtt = std::min( lowest_rtt, rtt );

    double est_lowest_owt = lowest_rtt / 2;
    double est_owt = ( skewed_owt - skewed_lowest_owt ) + est_lowest_owt;

    if ( est_owt <= est_lowest_owt + up_delay_threshold_ ) {
        the_window_size += up_delay_window_delta_;
    }

    if ( est_owt >= est_lowest_owt + down_delay_threshold_ ) {
        the_window_size -= down_delay_window_delta_;
    }

    if ( the_window_size < 2 ) {
        the_window_size = 2;
    } else if ( the_window_size > 10000 ) {
        the_window_size = 10000;
    }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 50;
}
