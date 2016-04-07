/*
* DASHTree.cpp
*****************************************************************************
* Copyright (C) 2015, liberty_developer
*
* Email: liberty.developer@xmail.net
*
* This source code and its use and distribution, is subject to the terms
* and conditions of the applicable license agreement.
*****************************************************************************/

#include <string>
#include <cstring>

#include "DASHTree.h"
#include "../oscompat.h"
#include "../helpers.h"

using namespace dash;

static unsigned char HexNibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  else if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return 0;
}

DASHTree::DASHTree()
  :download_speed_(0.0)
  , parser_(0)
  , encryptionState_(ENCRYTIONSTATE_UNENCRYPTED)
{
  current_period_ = new DASHTree::Period;
  periods_.push_back(current_period_);
}

DASHTree::~DASHTree()
{
}

/*----------------------------------------------------------------------
|   expat start
+---------------------------------------------------------------------*/
static void XMLCALL
start(void *data, const char *el, const char **attr)
{
  DASHTree *dash(reinterpret_cast<DASHTree*>(data));

  if (dash->currentNode_ & DASHTree::SSMNODE_SSM)
  {
    if (dash->currentNode_ & DASHTree::SSMNODE_PROTECTION)
    {
      if (strcmp(el, "ProtectionHeader") == 0)
      {
        for (; *attr;)
        {
          if (strcmp((const char*)*attr, "SystemID") == 0)
          {
            if (strcmp((const char*)*(attr + 1), "{9A04F079-9840-4286-AB92-E65BE0885F95}") == 0)
            {
              dash->strXMLText_.clear();
              dash->currentNode_ |= DASHTree::SSMNODE_PROTECTIONHEADER| DASHTree::SSMNODE_PROTECTIONTEXT;
            }
            break;
          }
          attr += 2;
        }
      }
    }
    else if (dash->currentNode_ & DASHTree::SSMNODE_STREAMINDEX)
    {
      if (strcmp(el, "QualityLevel") == 0)
      {
        //<QualityLevel Index = "0" Bitrate = "150000" NominalBitrate = "150784" BufferTime = "3000" FourCC = "AVC1" MaxWidth = "480" MaxHeight = "272" CodecPrivateData = "000000016742E01E96540F0477FE0110010ED100000300010000030032E4A0093401BB2F7BE3250049A00DD97BDF0A0000000168CE060CC8" NALUnitLengthField = "4" / >
        //<QualityLevel Index = "0" Bitrate = "48000" SamplingRate = "24000" Channels = "2" BitsPerSample = "16" PacketSize = "4" AudioTag = "255" FourCC = "AACH" CodecPrivateData = "131056E598" / >
 
        std::string::size_type pos = dash->current_adaptationset_->base_url_.find("{bitrate}");
        if (pos == std::string::npos)
          return;
        
        dash->current_representation_ = new DASHTree::Representation();
        dash->current_representation_->url_ = dash->current_adaptationset_->base_url_;
        
        const char *bw = "0";
        
        for (; *attr;)
        {
          if (strcmp((const char*)*attr, "Bitrate") == 0)
            bw = (const char*)*(attr + 1);
          else if (strcmp((const char*)*attr, "FourCC") == 0)
            dash->current_representation_->codecs_ = (const char*)*(attr + 1);
          else if (strcmp((const char*)*attr, "MaxWidth") == 0)
            dash->current_representation_->width_ = static_cast<uint16_t>(atoi((const char*)*(attr + 1)));
          else if (strcmp((const char*)*attr, "height") == 0)
            dash->current_representation_->height_ = static_cast<uint16_t>(atoi((const char*)*(attr + 1)));
          else if (strcmp((const char*)*attr, "SamplingRate") == 0)
            dash->current_representation_->samplingRate_ = static_cast<uint32_t>(atoi((const char*)*(attr + 1)));
          else if (strcmp((const char*)*attr, "Channels") == 0)
            dash->current_representation_->channelCount_ = static_cast<uint8_t>(atoi((const char*)*(attr + 1)));
          else if (strcmp((const char*)*attr, "Index") == 0)
            dash->current_representation_->id = (const char*)*(attr + 1);
          else if (strcmp((const char*)*attr, "CodecPrivateData") == 0)
          {
            unsigned int sz = strlen((const char*)*(attr + 1)) >> 1;
            dash->current_representation_->codec_extra_data_.resize(sz);
            const char* run = (const char*)*(attr + 1);
            std::string::iterator rd(dash->current_representation_->codec_extra_data_.begin());
            while (sz--){ *rd++ = (HexNibble(*run++) << 4) + HexNibble(*run++); }
          }
          else if (strcmp((const char*)*attr, "NALUnitLengthField") == 0)
            dash->current_representation_->nalu_length_ = static_cast<uint8_t>(atoi((const char*)*(attr + 1)));
          attr += 2;
        }
        dash->current_representation_->url_.replace(pos, 9, bw);
        dash->current_representation_->bandwidth_ = atoi(bw);
        dash->current_adaptationset_->repesentations_.push_back(dash->current_representation_);
      }
      else if (strcmp(el, "c") == 0)
      {
        //<c n = "0" d = "20000000" / >
        for (; *attr;)
        {
          if (*(const char*)*attr == 'd')
          {
            dash->current_adaptationset_->segment_durations_.push_back(atoi((const char*)*(attr + 1)));
            break;
          }
          attr += 2;
        }
      }
    }
    else if (strcmp(el, "StreamIndex") == 0)
    {
      //<StreamIndex Type = "video" TimeScale = "10000000" Name = "video" Chunks = "3673" QualityLevels = "6" Url = "QualityLevels({bitrate})/Fragments(video={start time})" MaxWidth = "960" MaxHeight = "540" DisplayWidth = "960" DisplayHeight = "540">
      dash->current_adaptationset_ = new DASHTree::AdaptationSet();
      dash->current_period_->adaptationSets_.push_back(dash->current_adaptationset_);
      for (; *attr;)
      {
        if (strcmp((const char*)*attr, "Type") == 0)
          dash->current_adaptationset_->type_ =
          stricmp((const char*)*(attr + 1), "video") == 0 ? DASHTree::VIDEO
          : stricmp((const char*)*(attr + 1), "audio") == 0 ? DASHTree::AUDIO
          : DASHTree::NOTYPE;
        else if (strcmp((const char*)*attr, "TimeScale") == 0)
          dash->current_adaptationset_->timescale_ = atoi((const char*)*(attr + 1));
        else if (strcmp((const char*)*attr, "Chunks") == 0)
          dash->current_adaptationset_->segment_durations_.reserve(atoi((const char*)*(attr + 1)));
        else if (strcmp((const char*)*attr, "Url") == 0)
          dash->current_adaptationset_->base_url_ = (const char*)*(attr + 1);
        attr += 2;
      }
      dash->segcount_ = 0;
      dash->currentNode_ |= DASHTree::SSMNODE_STREAMINDEX;
    }
    else if (strcmp(el, "Protection") == 0)
    {
      dash->currentNode_ |= DASHTree::SSMNODE_PROTECTION;
      dash->encryptionState_ = DASHTree::ENCRYTIONSTATE_SUPPORTED;
	}
  }
  else if (strcmp(el, "SmoothStreamingMedia") == 0)
  {
    unsigned int timeScale = 0, duration = 0;
    dash->overallSeconds_ = 0;
    for (; *attr;)
    {
      if (strcmp((const char*)*attr, "TimeScale") == 0)
        timeScale = atoi((const char*)*(attr + 1));
      else if (strcmp((const char*)*attr, "Duration") == 0)
        duration = atoi((const char*)*(attr + 1));
      attr += 2;
    }
    if (timeScale)
      dash->overallSeconds_ = (double)duration / timeScale;
    dash->currentNode_ |= DASHTree::SSMNODE_SSM;
  }
}

/*----------------------------------------------------------------------
|   expat text
+---------------------------------------------------------------------*/
static void XMLCALL
text(void *data, const char *s, int len)
{
	DASHTree *dash(reinterpret_cast<DASHTree*>(data));

    if (dash->currentNode_  & DASHTree::SSMNODE_PROTECTIONTEXT)
      dash->strXMLText_ += std::string(s, len);
}

/*----------------------------------------------------------------------
|   expat end
+---------------------------------------------------------------------*/
static void XMLCALL
end(void *data, const char *el)
{
  DASHTree *dash(reinterpret_cast<DASHTree*>(data));

  if (dash->currentNode_ & DASHTree::SSMNODE_SSM)
  {
    if (dash->currentNode_ & DASHTree::SSMNODE_PROTECTION)
    {
      if (dash->currentNode_ & DASHTree::SSMNODE_PROTECTIONHEADER)
      {
        if (strcmp(el, "ProtectionHeader") == 0)
          dash->currentNode_ &= ~DASHTree::SSMNODE_PROTECTIONHEADER;
      }
      else if (strcmp(el, "Protection") == 0)
      {
        dash->currentNode_ &= ~(DASHTree::SSMNODE_PROTECTION| DASHTree::SSMNODE_PROTECTIONTEXT);
        dash->parse_protection();
      }
    }
    else if (dash->currentNode_ & DASHTree::SSMNODE_STREAMINDEX)
    {
      if (strcmp(el, "StreamIndex") == 0)
      {
        if (dash->current_adaptationset_->repesentations_.empty())
          dash->current_period_->adaptationSets_.pop_back();
        else
        {
          for (std::vector<DASHTree::Representation*>::iterator
            b(dash->current_adaptationset_->repesentations_.begin()),
            e(dash->current_adaptationset_->repesentations_.end()); b != e; ++b)
          {
            (*b)->segments_.resize(dash->current_adaptationset_->segment_durations_.size());
            std::vector<uint32_t>::iterator bsd(dash->current_adaptationset_->segment_durations_.begin());
            uint64_t cummulated = 0;
            for (std::vector<DASHTree::Segment>::iterator bs((*b)->segments_.begin()), es((*b)->segments_.end()); bs != es; ++bsd, ++bs)
            {
              bs->range_begin_ = ~0;
              bs->range_end_ = cummulated;
              cummulated += *bsd;
            }
          }
        }
        dash->currentNode_ &= ~DASHTree::SSMNODE_STREAMINDEX;
      }
    }
    else if (strcmp(el, "SmoothStreamingMedia") == 0)
      dash->currentNode_ &= ~DASHTree::SSMNODE_SSM;
  }
}

/*----------------------------------------------------------------------
|   expat protection start
+---------------------------------------------------------------------*/
static void XMLCALL
protection_start(void *data, const char *el, const char **attr)
{
	DASHTree *dash(reinterpret_cast<DASHTree*>(data));
}

/*----------------------------------------------------------------------
|   expat protection text
+---------------------------------------------------------------------*/
static void XMLCALL
protection_text(void *data, const char *s, int len)
{
}

/*----------------------------------------------------------------------
|   expat protection end
+---------------------------------------------------------------------*/
static void XMLCALL
protection_end(void *data, const char *el)
{
	DASHTree *dash(reinterpret_cast<DASHTree*>(data));
}

/*----------------------------------------------------------------------
|   DASHTree
+---------------------------------------------------------------------*/

void DASHTree::Segment::SetRange(const char *range)
{
  const char *delim(strchr(range, '-'));
  if (delim)
  {
    range_begin_ = strtoull(range, 0, 10);
    range_end_ = strtoull(delim + 1, 0, 10);
  }
  else
    range_begin_ = range_end_ = 0;
}

bool DASHTree::open(const char *url)
{
  parser_ = XML_ParserCreate(NULL);
  if (!parser_)
    return false;
  XML_SetUserData(parser_, (void*)this);
  XML_SetElementHandler(parser_, start, end);
  XML_SetCharacterDataHandler(parser_, text);
  currentNode_ = 0;
  strXMLText_.clear();

  bool ret = download(url);
  
  XML_ParserFree(parser_);
  parser_ = 0;

  return ret;
}

bool DASHTree::write_data(void *buffer, size_t buffer_size)
{
  bool done(false);
  XML_Status retval = XML_Parse(parser_, (const char*)buffer, buffer_size, done);

  if (retval == XML_STATUS_ERROR)
  {
    unsigned int byteNumber = XML_GetErrorByteIndex(parser_);
    return false;
  }
  return true;
}

bool DASHTree::has_type(StreamType t)
{
  if (periods_.empty())
    return false;

  for (std::vector<AdaptationSet*>::const_iterator b(periods_[0]->adaptationSets_.begin()), e(periods_[0]->adaptationSets_.end()); b != e; ++b)
    if ((*b)->type_ == t)
      return true;
  return false;
}

uint32_t DASHTree::estimate_segcount(uint32_t duration, uint32_t timescale)
{
  double tmp(duration);
  duration /= timescale;
  return static_cast<uint32_t>((overallSeconds_ / duration)*1.01);
}

void DASHTree::parse_protection()
{
  if (strXMLText_.empty())
    return;

  //(p)repair the content
  std::string::size_type pos = 0;
  while ((pos = strXMLText_.find('\n', 0)) != std::string::npos)
    strXMLText_.erase(pos, 1);
  
  unsigned int buffer_size = strXMLText_.size();
  uint8_t *buffer = (uint8_t*)malloc(buffer_size);
  if (!b64_decode(strXMLText_.c_str(), buffer_size, buffer, buffer_size))
  {
    free(buffer);
    return;
  }

  XML_Parser pp = XML_ParserCreate(NULL);
  if (!pp)
  {
    free(buffer);
    return;
  }

  XML_SetUserData(pp, (void*)this);
  XML_SetElementHandler(pp, protection_start, protection_end);
  XML_SetCharacterDataHandler(pp, protection_text);
  
  bool done(false);
  XML_Parse(pp, strXMLText_.data(), strXMLText_.size(), done);

  XML_ParserFree(pp);
  free(buffer);

  strXMLText_.clear();
}
