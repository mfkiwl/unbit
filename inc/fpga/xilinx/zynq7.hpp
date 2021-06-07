/**
 * @file
 * @brief Common infrastructure for Xilinx Series-7 FPGAs
 */
#ifndef FPGA_XILINX_ZYNQ7_HPP_
#define FPGA_XILINX_ZYNQ7_HPP_ 1

#include "common.hpp"
#include "bram.hpp"

#include <array>
#include <string>

namespace fpga
{
	namespace xilinx
	{
		/**
		 * @brief Common infrastructure for Xilinx Series-7 FPGAs
		 */
		namespace v7
		{
			//--------------------------------------------------------------------------------------------------------------------
			/**
			 * @brief Description of a Zynq-7000 FPGA device
			 */
			class zynq7
			{
			protected:
				/**
				 * @brief Name of this device.
				 */
				const std::string name_;

				/**
				 * @brief IDCODE of this FPGA model.
				 */
				const uint32_t idcode_;

				/**
				 * @brief Number of block RAMs (RAMB36E1) in this device.
				 */
				const size_t num_brams_;

			protected:
				/**
				* @brief Constructs a Zynq-7 device.
				 */
				zynq7(const std::string& name, uint32_t idcode, size_t num_brams);

				/**
				 * @brief Disposes a Zynq-7 device.
				 */
				virtual ~zynq7() noexcept = 0;

			public:
				/**
				 * @brief Gets the name of this device.
				 */
				inline const std::string& name() const
				{
					return name_;
				}

				/**
				 * @brief Gets the IDCODE of this device.
				 */
				inline uint32_t idcode() const
				{
					return idcode_;
				}

				/**
				 * @brief Gets the number of block RAMs of this device.
				 */
				inline size_t num_brams(bram_category category) const
				{
					return (category == bram_category::ramb36) ? num_brams_ :
						(category == bram_category::ramb18) ? (2u * num_brams_) :
						0u;
				}

				/**
				 * @brief Gets a block RAM (RAMB36E1) by its index.
				 */
				virtual const bram& bram_at(bram_category category, size_t index) const = 0;

				/**
				 * @brief Gets a block RAM (RAMB36E1) by its X/Y coordinate
				 */
				const bram& bram_by_loc(bram_category category, unsigned x, unsigned y) const;

			public:
				/**
				 * @brief Gets the Zynq-7 FPGA for a given IDCODE.
				 */
				static const zynq7& get_by_idcode(uint32_t idcode);

			private:
				// Non-copyable
				zynq7(zynq7&) = delete;
				zynq7& operator=(zynq7&) = delete;
			};

			//--------------------------------------------------------------------------------------------------------------------
			/**
			 * @brief Detail implementation of a Zynq-7 variant
			 */
			template<uint32_t IdCode, size_t NumBrams>
			class zynq7_variant : public zynq7
			{
			private:
				/**
				 * @brief Block RAMs of this device. (RAMB36E1)
				 */
				const std::array<ramb36e1, NumBrams>& brams_;

				/**
				 * @brief Block RAM aliases of this device (RAMB18E1)
				 */
				const std::array<ramb18e1, 2u * NumBrams> brams_18_;

			private:
				/**
				 * @brief Helper method to create (and initialize) the RAMB18E1 aliases array.
				 */
				template<std::size_t... I>
				static inline const std::array<ramb18e1, sizeof...(I)>
				make_ramb18e1_aliases(const std::array<ramb36e1, sizeof...(I) / 2u>& brams, std::index_sequence<I...>)
				{
					return std::array<ramb18e1, sizeof...(I)> {
						ramb18e1 { brams[I / 2u], (I % 2u != 0u) }...
					};
				}

			public:
				/**
				 * @brief Constructs a Zynq-7 variant
				 */
				zynq7_variant(const std::string& name, const std::array<ramb36e1, NumBrams>& brams)
					: zynq7(name, IdCode, NumBrams), brams_(brams),
					  brams_18_(make_ramb18e1_aliases(brams, std::make_index_sequence<2u * NumBrams> {}))
				{
				}

				/**
				 * @brief Disposes a Zynq-7 variant
				 */
				~zynq7_variant() noexcept
				{
				}

			public:
				/**
				 * @brief Gets a block RAM by its index.
				 */
				const bram& bram_at(bram_category category, size_t index) const override
				{
					switch (category)
					{
					case bram_category::ramb36:
						return brams_.at(index);

					case bram_category::ramb18:
						return brams_18_.at(index);

					default:
						throw std::invalid_argument("selected block ram category is not support zynq-7 device");
					}
				}

			public:
				/**
				 * @brief Matches the IDCODE for this device variant.
				 */
				static bool match(uint32_t idcode)
				{
					return (IdCode == idcode);
				}
			};

			//--------------------------------------------------------------------------------------------------------------------
			// XC7Z010
			//
			struct xc7z010 final
			{
				/**
				 * @brief Tries to match the IDCODE of the XC7Z010 device model.
				 */
				static bool match(uint32_t idcode);

				/**
				 * @brief Gets the XC7Z020 device model.
				 *
				 * @return A reference to the XC7Z010 device model.
				 */
				static const zynq7& get();
			};

			//--------------------------------------------------------------------------------------------------------------------
			// XC7Z020
			//
			struct xc7z020 final
			{
				/**
				 * @brief Tries to match the IDCODE of the XC7Z020 device model.
				 */
				static bool match(uint32_t idcode);

				/**
				 * @brief Gets the XC7Z020 device model.
				 *
				 * @return A reference to the XC7Z020 device model.
				 */
				static const zynq7& get();
			};
		}
	}
}

#endif // #ifndef FPGA_XILINX_ZYNQ7_HPP_
