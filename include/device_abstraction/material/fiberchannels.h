#ifndef FIBERCHANNELS_H
#define FIBERCHANNELS_H

#include <cassert>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace dal {    ////////////////////////////////////////////////////////////////

template<typename T>
class DecoChannel
{
  public:
    DecoChannel( const DecoChannel& )            = delete;
    DecoChannel& operator=( const DecoChannel& ) = delete;

    DecoChannel() = default;

    void set( T newData )
    {
        auto lck = std::lock_guard<std::mutex>{ m_dataMutex };
        m_data   = std::move( newData );
        ++m_epoch;
    }

    T get() const
    {
        const auto lck = std::lock_guard<std::mutex>{ m_dataMutex };
        return m_data;
    }

    std::optional<T> getNew( uint64_t& clientEpoch ) const
    {
        const auto lck = std::lock_guard<std::mutex>{ m_dataMutex };

        if ( clientEpoch != m_epoch ) {
            clientEpoch = m_epoch;
            return m_data;
        } else
            return {};
    }

  private:
    /**
     * @note m_epoch overflow is not handled
     */
    uint64_t m_epoch{ 0 };
    T m_data;
    mutable std::mutex m_dataMutex;
};

//------------------------------------------------------------------------------
/**
 * @brief The MultiChannel class
 * @abstract unites unrelated DecoChannels
 */
template<typename Key, typename T>
class MultiChannel
{
  public:
    /**
     * @brief The DataTraceID class
     * @abstract a client descriptor storing information about the latest data received by the
     * client
     * @details used to eliminate duplicate data when accessing multiple channels
     * @attention thread-unsafe class
     */
    class DataTraceID
    {
        friend class MultiChannel<Key, T>;

      public:
        DataTraceID() = default;

        DataTraceID( const DataTraceID& )            = delete;
        DataTraceID& operator=( const DataTraceID& ) = delete;

        DataTraceID( DataTraceID&& other ) noexcept : m_versions( std::move( other.m_versions ) ) {}
        DataTraceID& operator=( DataTraceID&& ) = delete;

      private:
        std::unordered_map<Key, uint64_t>
            m_versions;    // used to prevent the re-capture of the same data
    };

  public:
    MultiChannel( const MultiChannel& )            = delete;
    MultiChannel& operator=( const MultiChannel& ) = delete;

    MultiChannel( std::vector<Key> channelKeys );

    std::shared_ptr<DecoChannel<T>> getChannel( Key k ) const;
    bool hasChannel( Key k ) const { return m_chnls.find( k ) != m_chnls.end(); }

    std::unordered_map<Key, T> fetchNew( DataTraceID& descriptor );

  private:
    std::unordered_map<Key, std::shared_ptr<DecoChannel<T>>> m_chnls;
};

//------------------------------------------------------------------------------

template<typename Key, typename T>
inline MultiChannel<Key, T>::MultiChannel( std::vector<Key> channelKeys )
{
    for ( const auto& key : channelKeys ) {
        m_chnls.emplace( key, std::make_shared<DecoChannel<T>>() );
    }
}

template<typename Key, typename T>
std::shared_ptr<DecoChannel<T>> MultiChannel<Key, T>::getChannel( Key k ) const
{
    auto chnlItr = m_chnls.find( k );

    if ( chnlItr == m_chnls.end() )
        throw std::out_of_range{ "MultiChannel bad channel access" };

    return chnlItr->second;
}

template<typename Key, typename T>
std::unordered_map<Key, T> MultiChannel<Key, T>::fetchNew( DataTraceID& descriptor )
{
    auto res = std::unordered_map<Key, T>{};

    for ( const auto& [key, chnlPtr] : m_chnls ) {
        auto& previousVersionItr = descriptor.m_versions[key];
        auto optionalData        = chnlPtr->getNew( previousVersionItr );

        if ( optionalData.has_value() )
            res.emplace( key, std::move( *optionalData ) );
    }

    return res;
}

//------------------------------------------------------------------------------

}    // namespace dal

#endif    // FIBERCHANNELS_H