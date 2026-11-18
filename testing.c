/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   testing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 20:05:38 by fandre-m          #+#    #+#             */
/*   Updated: 2026/11/18 20:05:38 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	*test_waiter(void *arg)
{
	t_coder	*coder;
	int		log_res;
	bool	wait_finished;

	coder = arg;
	log_res = log_state(coder, "is compiling", false);
	wait_finished = request_wait(coder->sim, 300);
	printf("Coder %d -> Log success: %d, wait success: %d\n",coder->coder_id, log_res, wait_finished);
	return (NULL);
}
