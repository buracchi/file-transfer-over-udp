#pragma once

#include "list.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_linked_list *cmn_linked_list_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief 
 * 
 * @return cmn_linked_list_t NULL on fail a pointer on success
 */
extern cmn_linked_list_t cmn_linked_list_init();
