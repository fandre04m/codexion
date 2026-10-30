/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   safe_thread.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/30 13:26:28 by fandre-m          #+#    #+#             */
/*   Updated: 2026/10/30 13:26:28 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	safe_pthread_create(pthread_t *thread, void *(*loop)(void *), void *coder)
{
	int	status;

	status = pthread_create(thread, NULL, loop, coder);
	if (status != 0)
		return (thread_error_handle(status, T_CREATE));
	return (status);
}

int	safe_pthread_join(pthread_t *thread)
{
	int	status;

	status = pthread_join(*thread, NULL);
	if (status != 0)
		return (thread_error_handle(status, T_JOIN));
	return (status);
}
