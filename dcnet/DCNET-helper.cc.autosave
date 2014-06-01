#include "DCNET-helper.h"
#include "DCNET.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/names.h"

namespace ns3 {

DCNETHelper::DCNETHelper (bool is_master_node,uint16_t port)
{
  m_factory.SetTypeId (DCNET::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
  SetAttribute ("MasterNode",BooleanValue  (is_master_node));
}

void 
DCNETHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DCNETHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DCNETHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DCNETHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
DCNETHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<DCNET> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
