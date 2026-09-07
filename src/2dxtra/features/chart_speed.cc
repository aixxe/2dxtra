#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <unordered_set>
#include <vector>

#include <MinHook.h>
#include <rubberband/RubberBandStretcher.h>

#include "../log.h"
#include "../game.h"
#include "../hooks/score_invalidator_hook.h"
#include "chart_speed.h"

namespace iidxtra::chart_speed
{
	// Sound bank entry; the game stores the block array directly in here.
	struct sound_entry_t
	{
		void* data;        // Block array
		void* data_end;
		std::size_t capacity;
	};

	// 16-byte header of every sound bank block; voice points to a VoiceImpl.
	struct sound_block_t
	{
		void* voice;
		std::uintptr_t alloc;
	};

	// Layout of the game's WaveFormatEx.
	struct wave_format_t
	{
		std::uint16_t wFormatTag;
		std::uint16_t nChannels;
		std::uint32_t nSamplesPerSec;
		std::uint32_t nAvgBytesPerSec;
		std::uint16_t nBlockAlign;
		std::uint16_t wBitsPerSample;
		std::uint16_t cbSize;
	};

	// Layout of BMSoundLib2017::WaveDataImpl.
	struct wave_data_t
	{
		void* vtable;
		std::uint64_t total_size;
		void (__cdecl* free_fn)(void*);
		void* alloc_base;
		std::uint8_t* riff_ptr;
		wave_format_t* fmt_ptr;
		std::uint64_t fmt_size;
		std::uint8_t* pcm_ptr;
		std::uint64_t pcm_bytes;
	};

	// Layout of BMSoundLib2017::PcmFeeder2chFloat.
	struct pcm_feeder_t
	{
		void* vtable;
		wave_data_t* wave;
		void* wave_ref;
		std::int32_t total_bytes;
		std::int32_t mode;
		float gain_a;
		float gain_b;
		std::uint8_t loop_enabled;
		std::uint8_t pad_29[3];
		std::int32_t loop_start_bytes;
		std::int32_t loop_end_bytes;
		std::int32_t current_bytes;
	};

	using get_sound_entry_fn = sound_entry_t* (*)(int);

	// chart speed multiplier; 1.00 = stock behavior
	float rate = 1.0f;

	// original function
	void* original_audio_load_fn = nullptr;

	auto reset() -> void
		{ rate = 1.0f; }

	auto mutate(const std::uint8_t player, std::vector<bm2dx::chart_event_t>& buffer) -> void
	{
		if (rate == 1.0f)
			return;

		// anything but the default invalidates the score
		score_invalidator_hook::invalidate(player);

		auto scaled = 0;

		// Scale event timings by 1 / rate, mirroring the native chart scaling
		// (ApplyRateModToChart @ 0x14017D980) exactly: float32 division, +0.4f,
		// truncate. Verified against bm2dx-2026080500.exe dumps: 0 mismatches
		// across 25 runs / 98102 events with the equivalent ratio arithmetic.
		//
		// The native also scales the note value with the same formula for
		// visible notes (type 0/1): a nonzero value is the hold length of a
		// freeze/charge note in ticks, so it must shrink with the timing or
		// holds outlast the compressed note spacing and become unhittable.
		// (The earlier "values untouched" observation was an artifact of the
		// dump charts having no hold notes.)
		for (auto& event: buffer)
		{
			event.offset = static_cast<std::int32_t>
				(std::trunc(static_cast<float>(event.offset) / static_cast<float>(rate) + 0.4f));

			if (event.type == bm2dx::chart_event_type::NOTE_P1 ||
				event.type == bm2dx::chart_event_type::NOTE_P2)
			{
				event.value = static_cast<std::int16_t>
					(std::trunc(static_cast<float>(static_cast<std::uint16_t>(event.value))
						/ static_cast<float>(rate) + 0.4f));
			}

			// the end-of-song terminator is scaled like every other event so the
			// chart finishes at the correct time, then iteration stops
			if (event.type == bm2dx::chart_event_type::END_OF_SONG)
				break;

			scaled++;
		}

		log::debug("Scaled {} chart events at x{:.2f}", scaled, rate);
	}

	static auto clamp_s16(const float sample) -> std::int16_t
	{
		auto scaled = std::lround(sample * 32767.0f);
		scaled = (std::max<long>)(-32768, (std::min<long>)(32767, scaled));
		return static_cast<std::int16_t>(scaled);
	}

	static auto scale_aligned_bytes(const std::int32_t value, const std::uint16_t block_align) -> std::int32_t
	{
		if (value <= 0)
			return value;

		auto scaled = static_cast<std::int32_t>(std::lround(static_cast<double>(value) / rate));
		if (block_align != 0)
			scaled -= scaled % block_align;
		return scaled;
	}

	// Update every feeder that references the given wave with the new size.
	static auto refresh_voice_feeders(std::uint8_t* voice,
		wave_data_t* wave,
		const std::uint64_t new_pcm_bytes,
		const std::uint16_t block_align) -> void
	{
		auto** begin = *reinterpret_cast<void***>(voice + 0x38);
		auto** end = *reinterpret_cast<void***>(voice + 0x40);
		if (!begin || !end)
			return;

		for (auto** it = begin; it != end; ++it)
		{
			auto* mixer = static_cast<std::uint8_t*>(*it);
			if (!mixer)
				continue;

			auto* feeder = *reinterpret_cast<pcm_feeder_t**>(mixer + 0x08);
			if (!feeder || feeder->wave != wave)
				continue;

			auto const old_total = feeder->total_bytes;
			auto const old_loop_start = feeder->loop_start_bytes;
			auto const old_loop_end = feeder->loop_end_bytes;
			auto const old_current = feeder->current_bytes;

			feeder->total_bytes = static_cast<std::int32_t>(new_pcm_bytes);

			if (feeder->loop_enabled)
			{
				feeder->loop_start_bytes = scale_aligned_bytes(old_loop_start, block_align);
				if (old_loop_end >= 0)
					feeder->loop_end_bytes = scale_aligned_bytes(old_loop_end, block_align);
				if (feeder->loop_end_bytes > feeder->total_bytes)
					feeder->loop_end_bytes = feeder->total_bytes;
				if (feeder->loop_start_bytes > feeder->loop_end_bytes)
					feeder->loop_start_bytes = feeder->loop_end_bytes;
			}

			if (old_current < 0)
				feeder->current_bytes = scale_aligned_bytes(old_current, block_align);
			else
				feeder->current_bytes = (std::min)(scale_aligned_bytes(old_current, block_align), feeder->total_bytes);

			log::debug("    feeder={:#x}: total {} -> {}, loop {} [{}..{}] -> [{}..{}], current {} -> {}",
				std::bit_cast<std::uintptr_t>(feeder),
				old_total,
				feeder->total_bytes,
				feeder->loop_enabled,
				old_loop_start,
				old_loop_end,
				feeder->loop_start_bytes,
				feeder->loop_end_bytes,
				old_current,
				feeder->current_bytes);
		}
	}

	// Replace the PCM of a wave with a freshly built RIFF/WAVE container.
	static auto rebuild_wave_data(wave_data_t* wave,
		std::vector<std::int16_t> const& pcm,
		const std::uint64_t pcm_bytes) -> bool
	{
		auto* fmt = wave->fmt_ptr;
		if (!fmt || !wave->free_fn || wave->fmt_size == 0)
			return false;

		auto const total_size = pcm_bytes + 28 + wave->fmt_size;
		auto const alloc_size = total_size + 16;
		auto* alloc_base = static_cast<std::uint8_t*>(_aligned_malloc(alloc_size, 16));
		if (!alloc_base)
			return false;

		auto const align_pad = ((total_size + 15) & ~std::uint64_t { 15 }) - total_size;
		auto* riff_ptr = alloc_base + align_pad;
		auto* p = riff_ptr;

		auto put_u32 = [&] (const std::uint32_t v)
		{
			std::memcpy(p, &v, sizeof(v));
			p += sizeof(v);
		};

		put_u32(0x46464952u); // "RIFF"
		put_u32(static_cast<std::uint32_t>(total_size - 8));
		put_u32(0x45564157u); // "WAVE"
		put_u32(0x20746d66u); // "fmt "
		put_u32(static_cast<std::uint32_t>(wave->fmt_size));
		auto* fmt_ptr = reinterpret_cast<wave_format_t*>(p);
		std::memcpy(p, fmt, wave->fmt_size);
		p += wave->fmt_size;
		put_u32(0x61746164u); // "data"
		put_u32(static_cast<std::uint32_t>(pcm_bytes));
		auto* pcm_ptr = p;
		std::memcpy(p, pcm.data(), pcm_bytes);

		if (wave->alloc_base)
			wave->free_fn(wave->alloc_base);

		wave->alloc_base = alloc_base;
		wave->riff_ptr = riff_ptr;
		wave->fmt_ptr = fmt_ptr;
		wave->pcm_ptr = pcm_ptr;
		wave->total_size = total_size;
		wave->pcm_bytes = pcm_bytes;
		return true;
	}

	auto audio_load_detour(std::int64_t sound_ctx, int sound_index, std::int64_t filepath_data, int flags) -> bool
	{
		auto result = reinterpret_cast<bool (*) (std::int64_t, int, std::int64_t, int)>
			(original_audio_load_fn)(sound_ctx, sound_index, filepath_data, flags);

		if (!result || rate == 1.0f)
			return result;

		auto* entry = reinterpret_cast<get_sound_entry_fn>(bm2dx::addr->GET_SOUND_ENTRY_FN)(sound_index);
		if (!entry || !entry->data || !entry->data_end)
		{
			log::debug("audio load hook: null entry for index {}", sound_index);
			return result;
		}

		auto const total_block_bytes = static_cast<std::size_t>
			(static_cast<std::uint8_t*>(entry->data_end) - static_cast<std::uint8_t*>(entry->data));

		if (total_block_bytes < sizeof(sound_block_t))
		{
			log::debug("audio load hook: block array too small: {}", total_block_bytes);
			return result;
		}

		auto* blocks = static_cast<sound_block_t*>(entry->data);
		auto const num_blocks = total_block_bytes / sizeof(sound_block_t);

		std::unordered_set<void*> seen_waves;
		auto processed = std::size_t { 0 };

		for (std::size_t i = 0; i < num_blocks; i++)
		{
			auto* voice = static_cast<std::uint8_t*>(blocks[i].voice);
			if (!voice)
				continue;

			auto* wave = *reinterpret_cast<wave_data_t**>(voice + 0x50);
			if (!wave || !seen_waves.insert(wave).second)
				continue;

			auto* fmt = wave->fmt_ptr;
			if (!fmt)
			{
				log::debug("  Block[{}]: null wave format", i);
				continue;
			}

			if (fmt->wFormatTag != 1 || fmt->wBitsPerSample != 16 ||
				fmt->nChannels <= 0 || !wave->pcm_ptr || wave->pcm_bytes == 0)
			{
				log::debug("  Block[{}]: unsupported format tag={:#x} bits={} channels={} pcm={:#x} bytes={}",
					i,
					fmt->wFormatTag,
					fmt->wBitsPerSample,
					fmt->nChannels,
					std::bit_cast<std::uintptr_t>(wave->pcm_ptr),
					wave->pcm_bytes);
				continue;
			}

			auto const old_pcm_bytes = wave->pcm_bytes;
			auto const old_block_align = fmt->nBlockAlign;
			auto const old_frames = static_cast<std::size_t>(old_pcm_bytes / old_block_align);
			if (old_frames == 0)
				continue;

			auto const start_frame = *reinterpret_cast<std::int32_t*>(voice + 0x60);
			auto const end_frame = *reinterpret_cast<std::int32_t*>(voice + 0x64);

			auto* src = reinterpret_cast<std::int16_t const*>(wave->pcm_ptr);
			auto const channels = static_cast<int>(fmt->nChannels);

			auto input_channels = std::vector<std::vector<float>>(channels);
			for (int ch = 0; ch < channels; ch++)
			{
				input_channels[ch].resize(old_frames);
				auto* dst = input_channels[ch].data();
				for (std::size_t frame = 0; frame < old_frames; frame++)
					dst[frame] = static_cast<float>(src[frame * channels + ch]) / 32768.0f;
			}

			// time-stretch the wave to the chart speed while preserving pitch.
			// Mirrors the native bm2dx-2026080500.exe implementation (stretcher
			// factory sub_140355E40 + drive loop sub_140356980):
			//   - ctor options = OptionProcessRealTime (1); NO study pass
			//   - setMaxProcessSize(1024); 1024-frame process chunks, final on last
			//   - output capped at trunc(time_ratio * input_frames)
			//   - drain loop: available() clamped to 1024 per retrieve, until 0/-1
			auto const time_ratio = 1.0 / rate;
			RubberBand::RubberBandStretcher stretcher(
				static_cast<std::size_t>(fmt->nSamplesPerSec),
				static_cast<std::size_t>(channels),
				RubberBand::RubberBandStretcher::OptionProcessRealTime,
				time_ratio,
				1.0);
			stretcher.setTimeRatio(time_ratio);
			stretcher.setPitchScale(1.0);
			stretcher.setMaxProcessSize(1024);

			auto const out_cap = static_cast<std::int32_t>(
				std::trunc(static_cast<double>(old_frames) * time_ratio));

			std::vector<float*> output_ptrs(channels);
			std::vector<std::vector<float>> output_channels(channels);
			for (int ch = 0; ch < channels; ch++)
				output_channels[ch].resize(1024);

			std::vector<std::int16_t> pcm_out;
			pcm_out.reserve(static_cast<std::size_t>(out_cap) * channels);

			auto drain = [&] ()
			{
				std::int32_t avail;
				while ((avail = stretcher.available()) > 0 &&
				       pcm_out.size() < static_cast<std::size_t>(out_cap) * channels)
				{
					auto n = std::min<std::int32_t>(avail, 1024);
					n = std::min(n, out_cap - static_cast<std::int32_t>(pcm_out.size() / channels));
					if (n <= 0)
						break;
					for (int ch = 0; ch < channels; ch++)
						output_ptrs[ch] = output_channels[ch].data();
					auto const retrieved = stretcher.retrieve(
						output_ptrs.data(), static_cast<std::size_t>(n));
					if (retrieved == 0)
						break;
					for (std::size_t frame = 0; frame < retrieved; frame++)
						for (int ch = 0; ch < channels; ch++)
							pcm_out.push_back(clamp_s16(output_ptrs[ch][frame]));
				}
			};

			std::size_t pos = 0;
			while (pos < old_frames)
			{
				auto const chunk = (std::min)(std::size_t { 1024 }, old_frames - pos);
				auto const is_final = (pos + chunk >= old_frames);
				std::vector<float const*> feed_ptrs(channels);
				for (int ch = 0; ch < channels; ch++)
					feed_ptrs[ch] = input_channels[ch].data() + pos;
				stretcher.process(const_cast<const float**>(feed_ptrs.data()), chunk, is_final);
				pos += chunk;
				drain();
			}
			drain();

			auto const out_frames = pcm_out.size() / channels;
			if (out_frames == 0)
			{
				log::debug("  Block[{}]: rubberband produced no output", i);
				continue;
			}

			auto const new_pcm_bytes = static_cast<std::uint64_t>(pcm_out.size() * sizeof(std::int16_t));
			if (!rebuild_wave_data(wave, pcm_out, new_pcm_bytes))
			{
				log::debug("  Block[{}]: failed to rebuild wave data", i);
				continue;
			}

			refresh_voice_feeders(voice, wave, new_pcm_bytes, old_block_align);

			auto const new_frames = static_cast<std::int32_t>(new_pcm_bytes / old_block_align);
			auto scaled_start = static_cast<std::int32_t>(std::lround(static_cast<double>(start_frame) / rate));
			auto scaled_end = static_cast<std::int32_t>(std::lround(static_cast<double>(end_frame) / rate));
			scaled_start = (std::max)(0, (std::min)(scaled_start, new_frames));
			scaled_end = (std::max)(scaled_start, (std::min)(scaled_end, new_frames));
			*reinterpret_cast<std::int32_t*>(voice + 0x60) = scaled_start;
			*reinterpret_cast<std::int32_t*>(voice + 0x64) = scaled_end;
			*reinterpret_cast<std::int32_t*>(voice + 0x88) = -1;

			processed++;
		}

		log::debug("audio load hook: stretched {} unique waves for index {}", processed, sound_index);
		return result;
	}

	auto install_hook() -> void
		{ MH_CreateHook(bm2dx::addr->LOAD_AUDIO_FN, reinterpret_cast<LPVOID>(audio_load_detour), &original_audio_load_fn); }
}
