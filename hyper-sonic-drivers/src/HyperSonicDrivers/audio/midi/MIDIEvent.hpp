#pragma once

#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <cstdint>
#include <vector>
#include <memory>

namespace HyperSonicDrivers::audio::midi
{
// TODO: store it as uint32_t
class MIDIEvent
{
public:
    // TODO build from a message array of 3 or 2 char.
    explicit MIDIEvent() = default;
    ~MIDIEvent()         = default;

    uint32_t          delta_time = 0;
    MIDI_EVENT_type_u type       = {0};
    midi_vector_t     data;

    inline uint32_t toUint32() const noexcept
    {
        uint32_t b = type.val | (data[0] << 8);
        if (data.size() == 2)
            b |= (data[1] << 16);

        return b;
    }

    // removed abs_time as it is not a MIDIEvent.
    // uint32_t abs_time = 0; /// absolute ticks time derived from delta_time used for conversion.
};
}    // namespace HyperSonicDrivers::audio::midi
