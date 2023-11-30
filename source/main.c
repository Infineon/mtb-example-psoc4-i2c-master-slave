/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSoC 4 MCU I2C Master EzI2C 
*              Slave example
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/* Header file includes */
#include "cy_pdl.h"
#include "cybsp.h"
#include "I2CMaster.h"
#include "I2CSlave.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* LED Status */
#define OFF                     CYBSP_LED_STATE_OFF
#define ON                      CYBSP_LED_STATE_ON

/* Delay duration */
#define CMD_TO_CMD_DELAY        (1000UL)

/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 *
 *  The main function performs the following actions:
 *   1. Initializes the BSP
 *   2. Calls the functions to set up I2C Master and EZI2C Slave.
 *   3. Sets up the command packet to be sent to the slave.
 *   4. I2C Master send the packet to EZI2C Slave and received the status
 *      packet.
 *   5. Changes the status of the LED depending on the command received.
 *   6. Updates the command packet.
 *
 ******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    uint8_t cmd = ON;
    uint32_t status;
    uint8_t buffer[WRITE_PACKET_SIZE];

    /* Initiate and enable Slave and Master SCBs */
    status = initSlave();
    if(status != I2C_SUCCESS)
    {
        handle_error();
    }
    status = initMaster();
    if(status != I2C_SUCCESS)
    {
        handle_error();
    }

    /* Enable interrupts */
    __enable_irq();

    for(;;)
    {
        /* Create packet to be sent to slave.  */
        buffer[PACKET_ADDR_POS] = EZI2C_BUFFER_ADDRESS;
        buffer[PACKET_SOP_POS] = PACKET_SOP;
        buffer[PACKET_EOP_POS] = PACKET_EOP;
        buffer[PACKET_CMD_POS] = cmd;

        /* Send packet with command to the slave */
        if (TRANSFER_CMPLT == WritePacketToEzI2C(buffer, WRITE_PACKET_SIZE))
        {
            /* The below code is for slave function. It is implemented in
             * this code example so that the master function can be tested
             * without the need of one more kit.
             */

            /* Read response packet from the slave */
            if (TRANSFER_CMPLT == ReadStatusPacketFromEzI2C())
            {
                /* Next command to be written */
                cmd = (cmd == ON) ? OFF : ON;
            }

            /* Check the EZI2C Slave buffer for the received command. If
             * the received packet is valid, change the status of LED
             * based on the command received.
             */
            CheckEzI2Cbuffer();

            /* Give 1 Second delay between commands */
            Cy_SysLib_Delay(CMD_TO_CMD_DELAY);
        }
    }
}

/* [] END OF FILE */
