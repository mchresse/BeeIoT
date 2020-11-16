/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
 ___ _____ _   ___ _  _____ ___  ___  ___ ___
/ __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
\__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
|___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
embedded.connectivity.solutions===============

Description: LoRa MAC layer implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis ( Semtech ), Gregory Cristian ( Semtech ) and Daniel Jäckle ( STACKFORCE )
*/
#include <Arduino.h>
#include <stdlib.h>
#include <stdint.h>
// #include "utilities.h"

#include "BIoTaes.h"
#include "BIoTcmac.h"
#include "BIoTCrypto.h"


extern unsigned int	lflags;		// BeeIoT log flag field (masks see beeiot.h)
#define LOGLORAW	512		// 512: LoRa Init: BeeIoT-WAN (NwSrv class)
#define	BHLOG(m)	if(lflags & m)	// macro for Log evaluation (type: uint)


/*!
 * CMAC/AES Message Integrity Code (MIC) Block B0 size
 */
#define LORAMAC_MIC_BLOCK_B0_SIZE                   16

/*!
 * MIC field computation initial data
 */
static uint8_t MicBlockB0[] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                              };

/*!
 * Contains the computed MIC field.
 *
 * \remark Only the 4 first bytes are used
 */
static uint8_t Mic[16];

/*!
 * Encryption aBlock and sBlock
 */
static uint8_t aBlock[] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                          };
static uint8_t sBlock[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                          };



/*!
 * AES computation context variable
 */
static aes_context AesContext;

/*!
 * CMAC computation context variable
 */
static AES_CMAC_CTX AesCmacCtx[1];

void memset1( uint8_t *dst, uint8_t value, uint16_t size ){
    while( size-- )
        *dst++ = value;
}

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size ){
    while( size-- )
        *dst++ = *src++;
}

/*!
 * \brief Computes the LoRaMAC frame MIC field
 *
 * \param [IN]  buffer          Data buffer
 * \param [IN]  size            Data buffer size
 * \param [IN]  key             AES key to be used
 * \param [IN]  address         Frame address
 * \param [IN]  dir             Frame direction [0: uplink, 1: downlink]
 * \param [IN]  sequenceCounter Frame sequence counter
 * \param [OUT] mic Computed MIC field
 */
void LoRaMacComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address,
                        uint8_t dir, uint32_t sequenceCounter, uint32_t *mic )
{
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacComputeMic: buf=0x%02X%02X%02X%02X..., size=%i",
        buffer[0], buffer[1], buffer[2], buffer[3], size);
    BHLOG(LOGLORAW) Serial.printf(", key=0x%02X%02X%02X%02X..., address=0x%X\n", key[0],key[1],key[2],key[3], address);
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacComputeMic: dir=%i, SCnt=0x%02X\n", dir, sequenceCounter);


    MicBlockB0[5] = dir;

    MicBlockB0[6] = ( address ) & 0xFF;
    MicBlockB0[7] = ( address >> 8 ) & 0xFF;
    MicBlockB0[8] = ( address >> 16 ) & 0xFF;
    MicBlockB0[9] = ( address >> 24 ) & 0xFF;

    MicBlockB0[10] = ( sequenceCounter ) & 0xFF;
    MicBlockB0[11] = ( sequenceCounter >> 8 ) & 0xFF;
    MicBlockB0[12] = ( sequenceCounter >> 16 ) & 0xFF;
    MicBlockB0[13] = ( sequenceCounter >> 24 ) & 0xFF;

    MicBlockB0[15] = size & 0xFF;

    AES_CMAC_Init( AesCmacCtx );

    AES_CMAC_SetKey( AesCmacCtx, key );

    AES_CMAC_Update( AesCmacCtx, MicBlockB0, LORAMAC_MIC_BLOCK_B0_SIZE );

    AES_CMAC_Update( AesCmacCtx, buffer, size & 0xFF );

    AES_CMAC_Final( Mic, AesCmacCtx );

    *mic = ( uint32_t )( ( uint32_t )Mic[3] << 24 | ( uint32_t )Mic[2] << 16 | ( uint32_t )Mic[1] << 8 | ( uint32_t )Mic[0] );
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacComputeMic: (0x%x%x%x%x) -> MIC=0x%X\n",
        Mic[0], Mic[1], Mic[2], Mic[3], *mic);
}

void LoRaMacPayloadEncrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir,
                    uint32_t sequenceCounter, uint8_t *encBuffer )
{
    uint16_t i;
    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;

    memset1( AesContext.ksch, '\0', 240 );
    aes_set_key( key, 16, &AesContext );

    aBlock[5] = dir;

    aBlock[6] = ( address ) & 0xFF;
    aBlock[7] = ( address >> 8 ) & 0xFF;
    aBlock[8] = ( address >> 16 ) & 0xFF;
    aBlock[9] = ( address >> 24 ) & 0xFF;

    aBlock[10] = ( sequenceCounter ) & 0xFF;
    aBlock[11] = ( sequenceCounter >> 8 ) & 0xFF;
    aBlock[12] = ( sequenceCounter >> 16 ) & 0xFF;
    aBlock[13] = ( sequenceCounter >> 24 ) & 0xFF;


    while( size >= 16 )
    {
        BHLOG(LOGLORAW) Serial.printf("  LoRaMacPayloadEncrypt: BUF=0x");
        aBlock[15] = ( ( ctr ) & 0xFF );
        ctr++;
        aes_encrypt16( aBlock, sBlock, &AesContext );
        for( i = 0; i < 16; i++ )
        {
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
            BHLOG(LOGLORAW) Serial.print(encBuffer[bufferIndex+i],HEX);
        }
        size -= 16;
        bufferIndex += 16;
        BHLOG(LOGLORAW) Serial.println();
    }

    if( size > 0 )
    {
        BHLOG(LOGLORAW) Serial.printf("  LoRaMacPayloadEncrypt: BUF=0x");
        aBlock[15] = ( ( ctr ) & 0xFF );
        aes_encrypt16( aBlock, sBlock, &AesContext );
        for( i = 0; i < size; i++ )
        {
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
            BHLOG(LOGLORAW) Serial.print(encBuffer[bufferIndex+i],HEX);
        }
        BHLOG(LOGLORAW) Serial.println();
    }
}

void LoRaMacPayloadDecrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t address, uint8_t dir, uint32_t sequenceCounter, uint8_t *decBuffer )
{
    LoRaMacPayloadEncrypt( buffer, size, key, address, dir, sequenceCounter, decBuffer );
}

void LoRaMacJoinComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t *mic )
{
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacJoinComputeMic: buf=0x%02X%02X%02X%02X..., size=%i",
        buffer[0], buffer[1], buffer[2], buffer[3], size);
    BHLOG(LOGLORAW) Serial.printf(", key=0x%02X%02X%02X%02X...\n", key[0],key[1],key[2],key[3]);

    AES_CMAC_Init( AesCmacCtx );

    AES_CMAC_SetKey( AesCmacCtx, key );

    AES_CMAC_Update( AesCmacCtx, buffer, size & 0xFF );

    AES_CMAC_Final( Mic, AesCmacCtx );

    *mic = ( uint32_t )( ( uint32_t )Mic[3] << 24 | ( uint32_t )Mic[2] << 16 | ( uint32_t )Mic[1] << 8 | ( uint32_t )Mic[0] );
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacJoinComputeMic: (0x%x%x%x%x) -> MIC=0x%X\n",
        Mic[0], Mic[1], Mic[2], Mic[3], *mic);
}

void LoRaMacJoinDecrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *decBuffer )
{
    memset1( AesContext.ksch, '\0', 240 );
    aes_set_key( key, 16, &AesContext );
    aes_encrypt16( buffer, decBuffer, &AesContext );
    // Check if optional CFList is included
    if( size >= 16 )
    {
        aes_encrypt16( buffer + 16, decBuffer + 16, &AesContext );
    }
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacJoinDecrypt: decBuffer 0x%X %X %X %X %X %X ...\n",
            decBuffer[0], decBuffer[1], decBuffer[2], decBuffer[3], decBuffer[4], decBuffer[5]);
}

void LoRaMacJoinComputeSKeys( const uint8_t *key, const uint8_t *appNonce, uint16_t devNonce, uint8_t *nwkSKey, uint8_t *appSKey )
{
    uint8_t nonce[16];
    uint8_t *pDevNonce = ( uint8_t * )&devNonce;

    memset1( AesContext.ksch, '\0', 240 );
    aes_set_key( key, 16, &AesContext );

    memset1( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x01;
    memcpy1( nonce + 1, appNonce, 6 );
    memcpy1( nonce + 7, pDevNonce, 2 );
    aes_encrypt16( nonce, nwkSKey, &AesContext );

    memset1( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x02;
    memcpy1( nonce + 1, appNonce, 6 );
    memcpy1( nonce + 7, pDevNonce, 2 );
    aes_encrypt16( nonce, appSKey, &AesContext );
    BHLOG(LOGLORAW) Serial.printf("  LoRaMacJoinComputeSKeys: appSKey: 0x%X %X %X %X %X %X...\n",
        appSKey[0], appSKey[1], appSKey[2], appSKey[3], appSKey[4], appSKey[5]);
    }
