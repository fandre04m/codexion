/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_mutex.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 18:14:02 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/27 15:27:41 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <pthread.h>
#include <unistd.h>

int	safe_mutex_handle(t_mtx *mutex, t_mutex_op op)
{
	int	status;

	if (op == M_INIT)
		status = pthread_mutex_init(mutex, NULL);
	else if (op == M_LOCK)
		status = pthread_mutex_lock(mutex);
	else if (op == M_UNLOCK)
		status = pthread_mutex_unlock(mutex);
	else if (op == M_DESTROY)
		status = pthread_mutex_destroy(mutex);
	else
		return (error_msg("Wrong operation code used for mutex."));

	if (status != 0)
		return (handle_mutex_error(status, op));

	return (status);
}
