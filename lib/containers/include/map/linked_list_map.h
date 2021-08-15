/**
 * @brief This object inherit from the list object and is indeed polimorphyc to
 *  it, you can use all the function defined in list.h casting a
 *  a cmn_linked_list object.
 */
#pragma once

#include "map.h"

#include "list/linked_list.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_linked_list_map {
    struct cmn_map super;
	struct cmn_linked_list list;
};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief 
 * 
 * @param map 
 * @return int 
 */
extern int cmn_linked_list_map_init(struct cmn_linked_list_map* map);
