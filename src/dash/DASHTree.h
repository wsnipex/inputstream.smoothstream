/*
* DASHTree.h
*****************************************************************************
* Copyright (C) 2015, liberty_developer
*
* Email: liberty.developer@xmail.net
*
* This source code and its use and distribution, is subject to the terms
* and conditions of the applicable license agreement.
*****************************************************************************/

#pragma once

#include <vector>
#include <string>
#include <inttypes.h>
#include "expat.h"

namespace dash
{
  template <typename T>
  struct SPINCACHE
  {
    SPINCACHE() :basePos(0) {};

    size_t basePos;

    const T *operator[](size_t pos) const
    {
      size_t realPos = basePos + pos;
      if (realPos >= data.size())
      {
        realPos -= data.size();
        if (realPos == basePos)
          return 0;
      }
      return &data[realPos];
    };

    size_t pos(const T* elem) const
    {
      size_t realPos = elem - &data[0];
      if (realPos < basePos)
        realPos += data.size() - basePos;
      else
        realPos -= basePos;
      return realPos;
    };

    void insert(const T &elem)
    {
      data[basePos] = elem;
      ++basePos;
      if (basePos == data.size())
        basePos = 0;
    }

    std::vector<T> data;
  };

  class DASHTree
  {
  public:
    enum StreamType
    {
      NOTYPE,
      VIDEO,
      AUDIO,
      TEXT,
      STREAM_TYPE_COUNT
    };

    // Node definition
    struct Segment
    {
      void SetRange(const char *range);

      uint64_t range_begin_;
      uint64_t range_end_;
    };

    struct AdaptationSet;

    struct Representation
    {
      Representation() :timescale_(0), duration_(0), bandwidth_(0), samplingRate_(0), width_(0), height_(0),
        aspect_(1.0f), fpsRate_(0), fpsScale_(1), channelCount_(0), nalu_length_(0){};
      std::string url_;
      std::string id;
      std::string codecs_;
      std::string codec_extra_data_;
      uint32_t bandwidth_;
      uint32_t samplingRate_;
      uint16_t width_, height_;
      uint32_t fpsRate_, fpsScale_;
      float aspect_;
      uint8_t channelCount_;
      uint8_t nalu_length_;
      //SegmentList
      uint32_t duration_, timescale_;
      SPINCACHE<Segment> segments_;
      const Segment *get_next_segment(const Segment *seg)const
      {
        if (!seg)
          return segments_[0];
        else
          return segments_[segments_.pos(seg) +1];
      };

      const Segment *get_segment(uint32_t pos)const
      {
        return segments_[pos];
      };

      const uint32_t get_segment_pos(const Segment *segment)const
      {
        return segments_.pos(segment);
      }
    }*current_representation_;

    struct AdaptationSet
    {
      AdaptationSet() :type_(NOTYPE), timescale_(0), first_live_time_(0), last_live_time_(0) {};
      ~AdaptationSet(){ for (std::vector<Representation* >::const_iterator b(repesentations_.begin()), e(repesentations_.end()); b != e; ++b) delete *b; };
      StreamType type_;
      uint32_t timescale_;
      uint64_t first_live_time_;
      uint64_t last_live_time_;
      std::string language_;
      std::string mimeType_;
      std::string base_url_;
      std::string codecs_;
      std::vector<Representation*> repesentations_;
      SPINCACHE<uint32_t> segment_durations_;

      struct SegmentTemplate
      {
        std::string initialization;
        std::string media;
        unsigned int startNumber;
        unsigned int timescale, duration;
      }segtpl_;
    }*current_adaptationset_;

    struct Period
    {
      Period(){};
      ~Period(){ for (std::vector<AdaptationSet* >::const_iterator b(adaptationSets_.begin()), e(adaptationSets_.end()); b != e; ++b) delete *b; };
      std::vector<AdaptationSet*> adaptationSets_;
      std::string base_url_;
    }*current_period_;

    std::vector<Period*> periods_;
    std::string base_url_;

    /* XML Parsing*/
    XML_Parser parser_;
    uint32_t currentNode_;
    uint32_t segcount_;
    double overallSeconds_;
    bool isLive_;
    uint64_t minLiveTime_, maxLiveTime_;

    uint32_t bandwidth_;

    double download_speed_;
    uint64_t download_bytes;

    std::pair<std::string, std::string> pssh_, adp_pssh_;

    enum
    {
      ENCRYTIONSTATE_UNENCRYPTED = 0,
      ENCRYTIONSTATE_ENCRYPTED = 1,
      ENCRYTIONSTATE_SUPPORTED = 2
    };
    unsigned int  encryptionState_;
    std::string protection_key_;
    uint8_t adpChannelCount_;
    
    enum
    {
      SSMNODE_SSM = 1 << 0,
      SSMNODE_PROTECTION = 1 << 1,
      SSMNODE_STREAMINDEX = 1 << 2,
      SSMNODE_PROTECTIONHEADER = 1 << 3,
      SSMNODE_PROTECTIONTEXT = 1 << 4
    };
    std::string strXMLText_;

    DASHTree();
    ~DASHTree();
    bool open(const char *url);
    bool has_type(StreamType t);
    uint32_t estimate_segcount(uint32_t duration, uint32_t timescale);
    void parse_protection();
    double get_download_speed() const { return download_speed_; };
    bool empty(){ return !current_period_ || current_period_->adaptationSets_.empty(); };
    const AdaptationSet *GetAdaptationSet(unsigned int pos) const { return current_period_ && pos < current_period_->adaptationSets_.size() ? current_period_->adaptationSets_[pos] : 0; };
    void SetFragmentDuration(const AdaptationSet* adp, size_t pos, uint32_t fragmentDuration);
  protected:
    virtual bool download(const char* url){ return false; };
    bool write_data(void *buffer, size_t buffer_size);
  };
}
