/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DCNET_H
#define DCNET_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

#include "cryptopp/aes.h" 
#include "cryptopp/sha.h"
#include "cryptopp/modes.h"
#include "cryptopp/integer.h"
#include "cryptopp/osrng.h"
#include "cryptopp/nbtheory.h"
#include <stdexcept>
#include "cryptopp/dh.h"
#include "cryptopp/secblock.h"
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <map>
#include <sstream>
#include <iomanip>


using CryptoPP::AutoSeededRandomPool;
using CryptoPP::ModularExponentiation;
using std::runtime_error;
using CryptoPP::DH;
using CryptoPP::SecByteBlock;
using CryptoPP::StringSink;
using CryptoPP::HexEncoder;
using std::map;
using std::pair;

using namespace CryptoPP;

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications 
 * \defgroup requestresponse RequestResponse
 */

/**
 * \ingroup requestresponse
 * \brief A Request Response server
 *
 * Every packet received is sent back.
 */
class DCNET : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  DCNET ();
  virtual ~DCNET ();

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void  GeneratePublickeys();
  void  GenerateCoinFlips();
  void ReceiveCoinFlip (Ptr<Packet> recPacket,Address from);
  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void SendDCNETQueryMessage();
  void HandleBroadcastRead (Ptr<Socket> socket);
  void HandleUnicastRead (Ptr<Socket> socket);
  void SendConfirmation(bool);
  void SendPublicKeys(Ptr<Packet>, int nodeid);
  void ReceivePublicKeys(Ptr<Packet>,Address,uint8_t *);
  void generateKeys(int index);
  void SendCoinFlips(std::string message,int neighbor_node);
  void GenerateAnnouncementsForRing(std::string);
  void SendAnnouncementForRing(std::string result, int immediate_neighbor);
  void split(std::map<ns3::Ipv4Address,int> &, std::map<int,ns3::Ipv4Address> &,const std::string &, char );
  void ReSendCoinFlip(Ptr<Packet>,Address);
  void SendPublicKeyRecvdToMaster();
  void BroadcastFlipMessage();
  void SendPrngRecvdToMaster();
  void BroadcastAnnouncementMessage();
  void GenerateAnnouncementsForFullyConnected();
  void SendAnnouncementForFullyConnected(std::string,int);
  void CalculateFinalAnnouncement();
  uint16_t m_port; //!< Port on which we listen/ broadcast  incoming packets.
  uint16_t m_unicast_port; //!< Port on which we listen/ broadcast  incoming packets.
  bool m_is_master_node;
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ptr<Socket> m_unicast_socket; //!< IPv4 Socket
  bool m_accept_dcnet;
  std::map<ns3::Ipv4Address,int> m_nodeaddr_nodeid;
  std::map<int,ns3::Ipv4Address> m_nodeid_nodeaddr;
  std::vector<int >m_public_keys_neighbors;
  Address m_local; //!< local  address
  EventId         m_sendBroadcastEvent;     //!< Event id for next start or stop event
  EventId           m_sendConfirmationEvent;
 
  bool m_publickey_gen;
  double m_time_wait_dcnet;
  std::map<int,SecByteBlock> m_publicKeyMap;
  std::map<int,SecByteBlock> m_privateKeyMap;
  std::map<int,SecByteBlock> m_sharedKeyMap;
  std::map<int,std::string> m_prngstringMap;
  std::map<int,std::string> m_announcementMap;
  int m_pub_keys_recv;
  int m_prng_str_recv;
  int m_my_nodeid;
  int m_my_start_cluster_node;
  int m_dhAgreedLength;
  unsigned m_message_length; 
  int m_master_nodeid; 
  int m_rank;
  int m_immediate_neighbor;
  int m_num_pub_keys_confirm;
  int m_num_coin_ex_confirm;
  bool m_dcnet_start;
  std::string m_topology;
  unsigned m_num_message_repeat;
  std::string m_final_message;
  const static int MESSAGE_DCNET_QUERY=200;
  std::string m_message;
  const static int MESSAGE_DCNET_ACCEPTANCE=201;
  const static int MESSAGE_DCNET_INITIATE=202;
  const static int MESSAGE_DCNET_PUBLIC_KEYS=203;
  const static int MESSAGE_DCNET_PRNG_STRING=204;
  const static int MESSAGE_DCNET_ANOUNCEMENT=205;
  const static int MESSAGE_DCNET_FINAL_ANOUNCEMENT=206;
  const static int MESSAGE_DCNET_RE_PRNG_STRING=207;
  const static int MESSAGE_DCNET_RECVD_PUB_KEYS=208;
  const static int MESSAGE_DCNET_EX_COIN_FLIPS=209;
  const static int MESSAGE_DCNET_RCVD_COIN_FLIPS=210;
  const static int MESSAGE_DCNET_EX_ANNOUNCEMENTS=211;
};

} // namespace ns3

#endif /*DCNET_H */
