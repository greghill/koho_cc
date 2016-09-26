#include <iostream>

#include "controller.hh"
#include "timestamp.hh"

using namespace std;

/* Default constructor */
Controller::Controller( const bool debug )
  : debug_( debug )
  , the_window_size(16)
  , skewed_lowest_owt(99999)
  , lowest_rtt(99999)
{}

/* Get current window size, in datagrams */
unsigned int Controller::window_size( void )
{
  if ( debug_ ) {
    cerr << "At time " << timestamp_ms()
        << " window size is " << the_window_size << endl;
  }

  return the_window_size;
}

/* A datagram was sent */
void Controller::datagram_was_sent( const uint64_t sequence_number,
				    /* of the sent datagram */
				    const uint64_t send_timestamp )
                                    /* in milliseconds */
{
  /* Default: take no action */

  if ( debug_ ) {
    cerr << "At time " << send_timestamp
	 << " sent datagram " << sequence_number << endl;
  }
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
    int64_t skewed_owt = recv_timestamp_acked - send_timestamp_acked;
    if (skewed_owt < skewed_lowest_owt)
        skewed_lowest_owt = skewed_owt;

    double rtt =  timestamp_ack_received - send_timestamp_acked;
    if (rtt < lowest_rtt)
        lowest_rtt = rtt;

    double est_lowest_owt = (lowest_rtt/2);
    double est_owt = (skewed_owt - skewed_lowest_owt) + est_lowest_owt;

    if (est_owt > 1.45 * est_lowest_owt)
        the_window_size -= .25;
    else
        the_window_size += .25;

    if (the_window_size < 2)
        the_window_size = 2;
    else if (the_window_size > 100)
        the_window_size = 100;

  if ( debug_ ) {
    cerr << "At time " << timestamp_ack_received
	 << " received ack for datagram " << sequence_number_acked
	 << " (send @ time " << send_timestamp_acked
	 << ", received @ time " << recv_timestamp_acked << " by receiver's clock)"
	 << endl;
  }
}

/* How long to wait (in milliseconds) if there are no acks
   before sending one more datagram */
unsigned int Controller::timeout_ms( void )
{
  return 50;
}
