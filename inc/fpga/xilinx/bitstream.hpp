/**
 * @file
 * @brief Common infrastructure for Xilinx Series-7 FPGAs
 */
#ifndef FPGA_XILINX_BITSTREAM_HPP_
#define FPGA_XILINX_BITSTREAM_HPP_ 1

#include "common.hpp"

namespace fpga
{
	namespace xilinx
	{
		namespace v7
		{

			//--------------------------------------------------------------------------------------------------------------------
			/**
			 * @brief Bitstream manipulation for Series-7 FPGAs
			 */
			class bitstream
			{
			public:
				/** Storage vector type. */
				typedef std::vector<uint8_t> data_vector;

				/** Byte data iterator (non-const) */
				typedef data_vector::iterator byte_iterator;

				/** Byte data iterator (const) */
				typedef data_vector::const_iterator const_byte_iterator;

				/** Type of bitstream/configuration data to be loaded */
				enum format
				{
					/** @brief Bitstream (.bit) format with configuration headers */
					bit = 1u,

					/** @brief Raw (.bin) format only containing the configuration frames */
					raw = 2u
				};

				/** Bitstream command packet info */
				struct packet
				{
					/**
					 * @brief Zero-based index of the (sub-)bitstream to which this packet belongs.
					 *
					 * @note Virtex-7 style FPGA bitstreams seem to allow some leeway with respect
					 *   to (sub-)bitstreams. Observations on bitstreams for larger FPGA show that
					 *   there is no 1:1 correspondence between this index and the SLR it (may)
					 *   configure.
					 *
					 *   Tools wishing to parse the config frames for SLRs can do so by tracking
					 *   a change in the stream index and the corresponding FDRI write packets.
					 *   (OBSERVATION: On devices with 3 SLRs there seem to be at least 3 substreams,
					 *    with FDRI write packets)
					 */
					std::size_t stream_index;

					/**
					 * @brief Position of this packet within its enclosing file/buffer storage.
					 *
					 * @note This offset tracks the packet's start position with respect to the
					 *   physical storage container (i.e. it counts the number of bytes since
					 *   including the size of all previous SLRs).
					 */
					std::size_t storage_offset;

					/**
					 * @brief Position of this packet in its enclosing (sub-)bitstream.
					 */
					std::size_t offset;

					/** @brief The raw command header word */
					uint32_t hdr;

					/**
					 * @brief Type of decoded packet
					 */
					uint32_t packet_type;

					/**
					 * @brief Opcode extracted from the packet
					 *
					 * @note The bitstream parser tracks the relationship between type1/type2 packets.
					 *   For type2 packets this field is filled from the previous type1 packet (or
					 *   0x00000000 if no previous type1 packet exists).
					 */
					uint32_t op;

					/**
					 * @brief Register operand extracted from the packet
					 *
					 * @note The bitstream parser tracks the relationship between type1/type2 packets.
					 *   For type2 packets this field is filled from the previous type1 packet (or
					 *   0xffffffff if no previous type1 packet exists).
					 */
					uint32_t reg;

					/** @brief Number of payload words of the packet */
					uint32_t word_count;

					/** @brief Start iterator for payload data */
					const_byte_iterator payload_start;

					/** @brief End iterator for payload data */
					const_byte_iterator payload_end;
				};

			private:
				/** @brief Byte offset of the first byte following the sync word. */
				size_t sync_offset_;

				/** @brief Byte offset of the first byte of the config frames are. */
				size_t frame_data_offset_;

				/** @brief Size of the config frame data in bytes. */
				size_t frame_data_size_;

				/** @brief IDCODE extracted from the bitstream (or 0xFFFFFFFF if no IDCODE was found) */
				uint32_t idcode_;

				/** @brief Offset of the CRC check command (or 0xFFFFFFFF if not present) */
				uint32_t crc_check_offset_;

				/** @brief In-memory data of the bitstream */
				data_vector data_;

			public:
				/**
				 * @brief Loads an uncompressed (and unencrypted) bitstream from a given file.
				 *
				 * @param[in] filename specifies the name (and path) of the bitstream file to be loaded.
				 *
				 * @param[in] fmt specifies the expected bitstream data format.
				 *
				 * @param[in] idcode specifies the expected IDCODE value
				 *   (or 0xFFFFFFFF to indicate that the IDCODE value is to be read from the bitstream data).
				 *
				 * @return The loaded bitstream object.
				 */
				static bitstream load(const std::string& filename, format fmt, uint32_t idcode = 0xFFFFFFFFu);

				/**
				 * @brief Stores an uncompressed (and unencrypted) bitstream to a given file.
				 *
				 * @param[in] filename specifies the destiation filename (and path).
				 *
				 * @param[in] bs specifies the bitstream to be dumped.
				 */
				static void save(const std::string& filename, const bitstream& bs);

				/**
				 * @brief Parses the packets in a bitstream.
				 *
				 * @note Documentation on the bitstream format of Xilix 7-Series bitstreams can
				 *   be found in  [Xilinx UG470; "Bitstream Composition"].
				 *
				 * @param[in] filename specifies the filename of the bitstream to be parsed.
				 */
				static void parse(const std::string& filename, std::function<bool(const packet&)> callback);

				/**
				 * @brief Parses the packets in a bitstream.
				 *
				 * @note Documentation on the bitstream format of Xilix 7-Series bitstreams can
				 *   be found in  [Xilinx UG470; "Bitstream Composition"].
				 *
				 * @param[in] stm specifies the input stream containing the bitstream data to parse.
				 */
				static void parse(std::istream& stm, std::function<bool(const packet&)> callback);

				/**
				 * @brief Parses the packets in a bitstream.
				 *
				 * @note Documentation on the bitstream format of Xilix 7-Series bitstreams can
				 *   be found in  [Xilinx UG470; "Bitstream Composition"].
				 *
				 * @param[in] start is an iterator indicates the start of bit-stream data.
				 *
				 * @param[in] end is an iterator indicates the end of bit-stream data.
				 *
				 * @param[in] base_file_offset indicates the absolute byte offset of the byte referenced
				 *   by @p start with respect to its enclosing file/array. This parameter is used to calculate
				 *   the absolute byte offset of data packets with respect to the enclosing file/array.
				 *
				 * @param[in] stream_index indicates the index of the (sub-)bitstream for this parse operation.
				 *   This parameter is forwarded verbatimly to the callback.
				 *
				 * @param[in] callback specifies the packet handler callback. The callback function
				 *   is invoked for every packet in the stream. The return value of the callback indicates
				 *   whether parsing should continue after the call.
				 *
				 * @return An iterator indicating the end of bit-stream data that has been parsed by this
				 *   call. The returned iterator is equal to @p end if the complete input data has been
				 *   exhausted.
				 */
				static const_byte_iterator parse(const_byte_iterator start, const_byte_iterator end,
								 size_t base_file_offset, size_t slr,
								 std::function<bool(const packet&)> callback);

			public:
				/**
				 * @brief Constructs a bitstream from a given input stream.
				 *
				 * @param[in] stm is the input stream to read the bitstream data from. The input stream
				 *   should be opened in binary mode.
				 *
				 * @param[in] fmt specifies the expected bitstream data format.
				 *
				 * @param[in] idcode specifies the expected IDCODE value (or 0xFFFFFFFF to indicate
				 *   that the IDCODE value is to be read from the bitstream data).
				 */
				bitstream(std::istream& stm, format fmt, uint32_t idcode = 0xFFFFFFFFu);

				/**
				 * @brief Move constructor for bitstream objects.
				 */
				bitstream(bitstream&& other) noexcept;

				/**
				 * @brief Disposes a bitstream object and its resources.
				 */
				~bitstream() noexcept;

				/**
				 * @brief Gets the byte offset from the start of the bitstream data to the first byte of the FPGA
				 *   configuration frames.
				 */
				inline size_t frame_data_offset() const
				{
					return frame_data_offset_;
				}

				/**
				 * @brief Gets the size of the FPGA configuration frame data in bytes.
				 */
				inline size_t frame_data_size() const
				{
					return frame_data_size_;
				}

				/**
				 * @brief Gets a byte iterator to the begin of the config packets area. (const)
				 */
				const_byte_iterator config_packets_begin() const;

				/**
				 * @brief Gets a byte iterator to the end of the config packets area. (const)
				 */
				const_byte_iterator config_packets_end() const;

				/**
				 * @brief Gets a byte iterator to the begin of the frame data area. (non-const)
				 */
				inline byte_iterator frame_data_begin()
				{
					return data_.begin() + frame_data_offset_;
				}

				/**
				 * @brief Gets a byte iterator to the end of the frame data area. (non-const)
				 */
				inline byte_iterator frame_data_end()
				{
					return frame_data_begin() + frame_data_size_;
				}

				/**
				 * @brief Gets a byte iterator to the begin of the frame data area. (const)
				 */
				inline const_byte_iterator frame_data_begin() const
				{
					return data_.cbegin() + frame_data_offset_;
				}

				/**
				 * @brief Gets a byte iterator to the end of the frame data area. (const)
				 */
				inline const_byte_iterator frame_data_end() const
				{
					return frame_data_begin() + frame_data_size_;
				}

				/**
				 * @brief Updates the CRC (if present) of this bitstream.
				 */
				void update_crc();

				/**
				 * @brief Reads a bit from the frame data area.
				 *
				 * @param[in] bit_offset specifies the offset (in bits) relative to the start of the frame data area.
				 *   (the method internally handles 32-bit word swaps as needed)
				 *
				 * @return The read-back value of the bit.
				 */
				bool read_frame_data_bit(size_t bit_offset) const;

				/**
				 * @brief Writes a bit in the frame data area.
				 *
				 * @param[bit] bit_offset specifies the offset (in bits) relative to the start of the frame data area.
				 *   (the method internally handles 32-bit word swaps as needed)
				 *
				 * @param[in] value the value to write at the given location.
				 */
				void write_frame_data_bit(size_t bit_offset, bool value);

				/**
				 * @brief Gets the device IDCODE that was parsed from the bitstream's configuration packets.
				 */
				inline uint32_t idcode() const
				{
					return idcode_;
				}

				/**
				 * @brief Sabes this bitstream to disk.
				 *
				 * @param[in,out] stm specifies the output stream to save this bitstream to.
				 */
				void save(std::ostream& stm) const;

			private:
				/**
				 * @brief Helper to load a binary data array from an input stream.
				 *
				 * @return An byte data vector with the loaded binary data.
				 */
				static data_vector load_binary_data(std::istream& stm);

				/**
				 * @brief Performs a range check for a slice of the frame data range.
				 */
				void check_frame_data_range(size_t offset, size_t length) const;

				/**
				 * @brief Remaps a byte offset into the frame data area (adjusting for 32-bit word swaps as needed).
				 *
				 * @param[in] offset is the (byte) offset into the frame data area to be adjusted.
				 * @return The mapped offset (adjusted for 32-bit word swap if needed).
				 */
				size_t map_frame_data_offset(size_t offset) const;

			private:
				// Non-copyable
				bitstream(const bitstream& other) = delete;
				bitstream& operator=(const bitstream& other) = delete;
			};
		}
	}
}

#endif // FPGA_XILINX_BITSTREAM_HPP_
