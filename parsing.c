/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fandre-m <fandre-m@student.42porto.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/21 16:29:15 by fandre-m          #+#    #+#             */
/*   Updated: 2026/09/21 16:29:15 by fandre-m         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"
#include <limits.h>
#include <string.h>

static inline bool	ft_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

static int	ft_atoi(const char *nptr, int *value)
{
	int	sum;

	sum = 0;
	while (*nptr == ' ' || (*nptr >= 9 && *nptr <= 13))
		nptr++;
	if (*nptr == '-')
		return (error_msg("All numeric arguments need to be positive."));
	if (*nptr == '+')
		nptr++;
	if (!ft_isdigit(*nptr))
		return (error_msg("Invalid numeric argument detected."));
	while (ft_isdigit(*nptr))
	{
		if (sum > (INT_MAX - (*nptr - '0')) / 10)
			return (error_msg("One or more arguments exceed INT_MAX."));
		sum = (sum * 10) + (*nptr - '0');
		nptr++;
	}
	if (*nptr != '\0')
		return (error_msg("Invalid numeric argument detected."));
	*value = sum;
	return (0);
}

static int	validate_scheduler(char *str)
{
	if (strcmp(str, "fifo") != 0 && strcmp(str, "edf") != 0)
		return (error_msg("Scheduler must be 'fifo' or 'edf'."));
	return (0);
}

int	parse_input(t_sim *sim, char **av)
{
	int	i;
	int	*fields[7];

	fields[0] = &sim->n_coders;
	fields[1] = &sim->time_burnout;
	fields[2] = &sim->time_compile;
	fields[3] = &sim->time_debug;
	fields[4] = &sim->time_refact;
	fields[5] = &sim->req_compiles;
	fields[6] = &sim->dongle_cooldown;
	i = 0;
	while (i <= 6)
	{
		if (ft_atoi(av[i + 1], fields[i]))
			return (1);
		i++;
	}
	if (validate_scheduler(av[8]))
		return (1);
	sim->scheduler = av[8];
	return (0);
}
