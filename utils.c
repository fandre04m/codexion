/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/18 17:07:38 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/12 00:55:04 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <time.h>

long long	get_time_ms(void)
{
	struct timespec	now;

	
}

int	error_msg(const char *msg)
{
	fprintf(stderr, R "Error: %s\n" RST, msg);
	return (1);
}
