#include "dnsmessages.hh"
#include "dns-types.hh"

using namespace std;

dnsname DNSMessageReader::getName()
{
  dnsname name;
  for(;;) {
    uint8_t labellen=payload.getUInt8();
    if(labellen > 63)
      throw std::runtime_error("Got a compressed label");
    if(!labellen) // end of dnsname
      break;
    dnslabel label = payload.getBlob(labellen);
    name.push_back(label);
  }
  return name;
}

void DNSMessageReader::getQuestion(dnsname& name, DNSType& type)
{
  name=getName();
  type=(DNSType)payload.getUInt16();
}


void DNSMessageWriter::putRR(DNSSection section, const dnsname& name, DNSType type, uint32_t ttl, const std::unique_ptr<RRGen>& content)
{
  auto cursize = payloadpos;
  try {
    putName(name);
    putUInt16((int)type); putUInt16(1);
    putUInt32(ttl);
    auto pos = putUInt16(0); // placeholder
    content->toMessage(*this);
    putUInt16At(pos, payloadpos-pos-2);
  }
  catch(...) {
    payloadpos = cursize;
    throw;
  }
  switch(section) {
    case DNSSection::Question:
      throw runtime_error("Can't add questions to a DNS Message with putRR");
    case DNSSection::Answer:
      dh.ancount = htons(ntohs(dh.ancount) + 1);
      break;
    case DNSSection::Authority:
      dh.nscount = htons(ntohs(dh.nscount) + 1);
      break;
    case DNSSection::Additional:
      dh.arcount = htons(ntohs(dh.arcount) + 1);
      break;
  }
}

void DNSMessageWriter::setQuestion(const dnsname& name, DNSType type)
{
  dh.ancount = dh.arcount = dh.nscount = 0;
  payloadpos=0;
  putName(name);
  putUInt16((uint16_t)type);
  putUInt16(1); // class
}

string DNSMessageReader::serialize() const
{
  return string((const char*)this, (const char*)this + sizeof(dnsheader) + payload.payloadpos);
}
string DNSMessageWriter::serialize() const
{
  std::string ret((const char*)this, (const char*)this + sizeof(dnsheader));
  ret.append((const unsigned char*)&payload[0], (const unsigned char*)&payload[payloadpos]);
  return ret;
}

static_assert(sizeof(DNSMessageReader) == 516, "dnsmessagereader size must be 516");
