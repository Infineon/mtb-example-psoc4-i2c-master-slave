/******************************************************************************
* File Name:   I2CSlave.c
*
* Description: This file contains all the functions and variables required for
*              proper operation of EZI2C slave peripheral
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
#include "I2CSlave.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* EZI2C slave interrupt macros */
#define EZI2C_INTR_NUM          CYBSP_EZI2C_IRQ
#define EZI2C_INTR_PRIORITY         (3UL)

#define EZI2C_BUFFER_SIZE           (0x08UL)

/* Start and end of packet markers */
#define PACKET_SOP                  (0x01UL)
#define PACKET_EOP                  (0x17UL)

/* Command valid status */
#define STS_CMD_DONE                (0x00UL)
#define STS_CMD_FAIL                (0xFFUL)

/* Packet data position in buffer */
#define EzPACKET_SOP_POS            (0x00UL)
#define EzPACKET_CMD_POS            (0x01UL)
#define EzPACKET_EOP_POS            (0x02UL)
#define PACKET_RPLY_SOP_POS         (0x05UL)
#define PACKET_RPLY_STS_POS         (0x06UL)
#define PACKET_RPLY_EOP_POS         (0x07UL)

#define ZERO                        (0UL)

/*******************************************************************************
* Global variables
*******************************************************************************/

/** The instance-specific context structure.
 * It is used by the driver for internal configuration and
 * data keeping for the EZI2C. Do not modify anything in this structure.
 */
cy_stc_scb_ezi2c_context_t CYBSP_EZI2C_context;

/* CYBSP_EZI2C_SCB_IRQ */
cy_stc_sysint_t CYBSP_EZI2C_SCB_IRQ_cfg =
{
        /*.intrSrc =*/ CYBSP_EZI2C_IRQ,
        /*.intrPriority =*/ 3u
};

/* EZI2C buffer */
uint8_t buffer[EZI2C_BUFFER_SIZE] ;

/*******************************************************************************
* Function Declaration
*******************************************************************************/
void SEzI2C_InterruptHandler(void);

/*******************************************************************************
* Function Name: SEzI2C_InterruptHandler
****************************************************************************//**
*
* Summary:
*   Execute interrupt service routine.
*
*******************************************************************************/
void SEzI2C_InterruptHandler(void)
{
    /* ISR implementation for EZI2C. */
    Cy_SCB_EZI2C_Interrupt(CYBSP_EZI2C_HW, &CYBSP_EZI2C_context);
}

/*******************************************************************************
* Function Name: CheckEzI2Cbuffer
****************************************************************************//**
*
* Summary:
*   Continuously read the contents of EzI2C buffer. Compare the packets and
*   execute the command.
*
*******************************************************************************/
void CheckEzI2Cbuffer( void )
{
    uint32_t ezi2cState;

    /* Disable the EZI2C interrupts so that ISR is not serviced while
     * checking for EZI2C status.
     */
    NVIC_DisableIRQ(CYBSP_EZI2C_SCB_IRQ_cfg.intrSrc);

    /* Read the EZi2C status */
    ezi2cState = Cy_SCB_EZI2C_GetActivity(CYBSP_EZI2C_HW, &CYBSP_EZI2C_context);

    /* Write complete without errors: parse packets, otherwise ignore. */
    if((0u != (ezi2cState & CY_SCB_EZI2C_STATUS_WRITE1)) && (0u == (ezi2cState & CY_SCB_EZI2C_STATUS_ERR)))
    {
        /* Check buffer content to know any new packets are written from master. */
        if( ( buffer[EzPACKET_SOP_POS] ==PACKET_SOP) && ( buffer[EzPACKET_EOP_POS] ==PACKET_EOP) )
        {
            Cy_GPIO_Write(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_NUM, buffer[EzPACKET_CMD_POS]);

            /* Clear the location so that any new packets written to buffer will be known. */
            buffer[EzPACKET_SOP_POS]      = ZERO;
            buffer[EzPACKET_EOP_POS]      = ZERO;

            /* Write to buffer the data related to status. */
            buffer[PACKET_RPLY_SOP_POS] = PACKET_SOP;
            buffer[PACKET_RPLY_STS_POS] = STS_CMD_DONE;
            buffer[PACKET_RPLY_EOP_POS] = PACKET_EOP;
        }
        else
        {
            /* write to buffer the data related to status. */
            buffer[PACKET_RPLY_SOP_POS] = PACKET_SOP;
            buffer[PACKET_RPLY_STS_POS] = STS_CMD_FAIL;
            buffer[PACKET_RPLY_EOP_POS] = PACKET_EOP;
        }
    }
     /* Enable interrupts for servicing ISR. */
    NVIC_EnableIRQ(CYBSP_EZI2C_SCB_IRQ_cfg.intrSrc);
}


/*******************************************************************************
* Function Name: handle_error
****************************************************************************//**
*
* Summary:
*   Process unrecoverable errors such as any component initialization errors etc.
*   In case of such error the system will stay in the infinite loop of this
*   function.
*   Note that if error occurs interrupts are disabled.
*
*******************************************************************************/
void handle_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    /* Infinite loop. */
    while(1u) {}
}

/*******************************************************************************
* Function Name: initSlave
********************************************************************************
*
* Summary:
*   Initiate and enable the slave SCB
*
* Return:
*   Status of initialization.
*
*******************************************************************************/
uint32_t initSlave(void)
{
    cy_en_scb_ezi2c_status_t initEzI2Cstatus;
    cy_en_sysint_status_t syCYBSP_EZI2Cstatus;

    /*Initialize EzI2C slave.*/
    initEzI2Cstatus = Cy_SCB_EZI2C_Init(CYBSP_EZI2C_HW, &CYBSP_EZI2C_config, &CYBSP_EZI2C_context);
    if(initEzI2Cstatus!=CY_SCB_EZI2C_SUCCESS)
    {
        return I2C_FAILURE;
    }
    /* Hook interrupt service routine and enable interrupt. */
    syCYBSP_EZI2Cstatus = Cy_SysInt_Init(&CYBSP_EZI2C_SCB_IRQ_cfg, &SEzI2C_InterruptHandler);
    if(syCYBSP_EZI2Cstatus!=CY_SYSINT_SUCCESS)
    {
        return I2C_FAILURE;
    }
    NVIC_EnableIRQ((IRQn_Type) CYBSP_EZI2C_SCB_IRQ_cfg.intrSrc);

    /* Configure buffer for communication with master. */
    Cy_SCB_EZI2C_SetBuffer1(CYBSP_EZI2C_HW, buffer, EZI2C_BUFFER_SIZE, EZI2C_BUFFER_SIZE, &CYBSP_EZI2C_context);

    /* Enable SCB for the EZI2C operation. */
    Cy_SCB_EZI2C_Enable(CYBSP_EZI2C_HW);
    return I2C_SUCCESS;
}


