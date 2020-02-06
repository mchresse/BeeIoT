/*!
 * \file      LoRaMacCrypto.h
 *
 * \brief     LoRa MAC layer cryptography implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jäckle ( STACKFORCE )
 *
 * \defgroup    LORAMAC_CRYPTO  LoRa MAC layer cryptography implementation
 *              This module covers the implementation of cryptographic functions
 *              of the LoRaMAC layer.
 * \{
 */
#ifndef __LORAMAC_CRYPTO_H__
#define __LORAMAC_CRYPTO_H__

/*!
 * \brief Set size elements of dst array with value
 *
 * \remark STM32 Standard memset function only works on pointers that are aligned
 *
 * \param [OUT] dst   Destination array
 * \param [IN]  value Default value
 * \param [IN]  size  Number of bytes to be copied
 */
extern void memset1( uint8_t *dst, uint8_t value, uint16_t size );

/*!
 * \brief Copies size elements of src array to dst array
 *
 * \remark STM32 Standard memcpy function only works on pointers that are aligned
 *
 * \param [OUT] dst  Destination array
 * \param [IN]  src  Source array
 * \param [IN]  size Number of bytes to be copied
 */
extern void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size );

/*!
 * \brief Returns the minimum value between a and b
 *
 * \param [IN] a 1st value
 * \param [IN] b 2nd value
 * \retval minValue Minimum value
 */
#define MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )

/*!
 * Computes the LoRaMAC frame MIC field
 *
 * \param [IN]  buffer          - Data buffer
 * \param [IN]  size            - Data buffer size
 * \param [IN]  key             - AES key to be used
 * \param [IN]  address         - Frame address
 * \param [IN]  dir             - Frame direction [0: uplink, 1: downlink]
 * \param [IN]  sequenceCounter - Frame sequence counter
 * \param [OUT] mic             - Computed MIC field
 */
void LoRaMacComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint32_t *mic );

/*!
 * Computes the LoRaMAC payload encryption
 *
 * \param [IN]  buffer          - Data buffer
 * \param [IN]  size            - Data buffer size
 * \param [IN]  key             - AES key to be used
 * \param [IN]  address         - Frame address
 * \param [IN]  dir             - Frame direction [0: uplink, 1: downlink]
 * \param [IN]  sequenceCounter - Frame sequence counter
 * \param [OUT] encBuffer       - Encrypted buffer
 */
void LoRaMacPayloadEncrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *encBuffer );

/*!
 * Computes the LoRaMAC payload decryption
 *
 * \param [IN]  buffer          - Data buffer
 * \param [IN]  size            - Data buffer size
 * \param [IN]  key             - AES key to be used
 * \param [IN]  address         - Frame address
 * \param [IN]  dir             - Frame direction [0: uplink, 1: downlink]
 * \param [IN]  sequenceCounter - Frame sequence counter
 * \param [OUT] decBuffer       - Decrypted buffer
 */
void LoRaMacPayloadDecrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *decBuffer );

/*!
 * Computes the LoRaMAC Join Request frame MIC field
 *
 * \param [IN]  buffer          - Data buffer
 * \param [IN]  size            - Data buffer size
 * \param [IN]  key             - AES key to be used
 * \param [OUT] mic             - Computed MIC field
 */
void LoRaMacJoinComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t *mic );

/*!
 * Computes the LoRaMAC join frame decryption
 *
 * \param [IN]  buffer          - Data buffer
 * \param [IN]  size            - Data buffer size
 * \param [IN]  key             - AES key to be used
 * \param [OUT] decBuffer       - Decrypted buffer
 */
void LoRaMacJoinDecrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *decBuffer );

/*!
 * Computes the LoRaMAC join frame decryption
 *
 * \param [IN]  key             - AES key to be used
 * \param [IN]  appNonce        - Application nonce
 * \param [IN]  devNonce        - Device nonce
 * \param [OUT] nwkSKey         - Network session key
 * \param [OUT] appSKey         - Application session key
 */
void LoRaMacJoinComputeSKeys( const uint8_t *key, const uint8_t *appNonce, uint16_t devNonce, uint8_t *nwkSKey, uint8_t *appSKey );

/*! \} defgroup LORAMAC */

#endif // __LORAMAC_CRYPTO_H__
