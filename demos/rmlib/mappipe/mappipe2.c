/*------------------------------------------------------------------------
--                                                                      --
--           H E L I O S   N E T W O R K I N G   S O F T W A R E	--
--           ---------------------------------------------------	--
--                                                                      --
--           Copyright (c) 1990 - 1993, Perihelion Software Ltd.        --
--                        All Rights Reserved.                          --
--                                                                      --
-- mappipe2.c								--
--                                                                      --
--      Modified version of the mappipe example program.		--
--	Mappipe Author:   BLV 15/5/91					--
--	Mappipe2 Creator: NOJC 24/3/93					--
--                                                                      --
------------------------------------------------------------------------*/

/**
*** This program is a version of the mappipe program described in the
*** Resource Management library chapter of the book "The Helios Parallel
*** Operating System".
**/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue.h>
#include <rmlib.h>

/* prints a debugging message */

static void
debug( const char * message, ... )
{
  va_list args;

  va_start( args, message );

  fprintf( stderr, "mappipe2: " );
  vfprintf( stderr, message, args );
  fputc( '\n', stderr );
  va_end( args );

   return;

} /* debug */


/* prints an error message and then terminates the task force */

static void
fail( const char * message, ... )
{
  va_list args;

  va_start( args, message );

  fprintf( stderr, "mappipe2: " );
  vfprintf( stderr, message, args );
  fputc( '\n', stderr );
  va_end( args );

   exit( EXIT_FAILURE );

} /* fail */

/**----------------------------------------------------------------------------
*** Build a taskforce in the form of a pipeline. The start of the pipeline
*** is the single program 'ps' with the argument 'all'. Then there are
*** (n) copies of the 'cat' program.
**/
static RmTaskforce	build_taskforce(int number_tasks)
{ RmTaskforce	result	= RmNewTaskforce();
  RmTask	previous, current;
  int		i;

  if (result == (RmTaskforce) NULL) return((RmTaskforce) NULL);
  RmSetTaskforceId(result, "pipeline");
  
  previous = RmNewTask();
  if (previous == (RmTask) NULL) return((RmTaskforce) NULL);
  RmSetTaskId(previous, "start");
  RmSetTaskCode(previous, "/helios/bin/ps");
  RmAddTaskArgument(previous, 1, "all");
  RmAddtailTask(result, previous);

  for (i = 1; i <= number_tasks; i++)
   { char	buf[16];
     current	= RmNewTask();
     if (current == (RmTask) NULL) return((RmTaskforce) NULL);
     sprintf(buf, "worker%d", i);
     RmSetTaskId(current, buf);
     RmSetTaskCode(current, "/helios/bin/cat");
     RmAddtailTask(result, current);
     RmMakeChannel(previous, 1, current, 0);
     previous = current;
   }
   
  return(result);
} /* build_taskforce */

/**----------------------------------------------------------------------------
*** Obtain a network of processors in the form of a pipeline.
*** First, the number of tasks in the taskforce is counted and a suitable 
*** vector is allocated to hold a corresponding number of processors. Also,
*** a network is allocated to hold the resulting processors. 
***
*** Next, a copy of the whole network is obtained. This network must be
*** searched to find a suitable pipeline.
***
*** First, it is necessary to find a starting place for the pipeline.
*** Potentially any of the processors may be suitable, so the network is
*** searched in a loop.
***
*** For every possible starting place, the network is searched to find
*** a suitable pipeline of processors. If the recursive search succeeds
*** then the whole taskforce can be mapped executed. Otherwise an
*** alternative starting place must be considered.
**/

typedef	struct	{
	RmProcessor	Template;
	RmProcessor	Obtained;
} ProcessorVector;


/**
*** This routine is called to search the network for a suitable starting
*** place for the pipeline. This means it must satisfy several criteria.
*** 1) the processor must be unowned.
*** 2) the processor must be a T800 or C40 with at least one megabyte of 
***    memory.  The processor type restriction is spurious, but makes the
***    example slightly more interesting.
*** 3) the processor must be a normal Helios processor
*** 4) the processor must not have been used as a starting place already.
**/

static int find_starting_place(RmProcessor processor, ...)
{
  if (RmIsNetwork(processor))
   return(RmSearchNetwork((RmNetwork) processor, &find_starting_place));
   	
  if ((RmGetProcessorPrivate(processor)			!= 0)          ||
#ifdef __C40
      (RmGetProcessorType(processor)			!= RmT_C40)    ||
#else
      (RmGetProcessorType(processor)			!= RmT_T800)   ||
#endif
       (RmGetProcessorMemory(processor)			<  0x100000)   ||
      ((RmGetProcessorPurpose(processor) & RmP_Mask)	!= RmP_Helios) ||
      (RmGetProcessorOwner(processor)			!= RmO_FreePool))
    {
#if 0
      debug( "processor %s is unsuitable as a starting point",
		RmGetProcessorId( processor ) ); 
        }
#endif
      return(0);
    }

  RmSetProcessorPrivate(processor, 1);
  return((int) processor);
} /* find_starting_place */

/**
*** This routine is used to continue the search of the network for
*** a suitable pipeline. All the links of the previous processor
*** are considered. If the neighbouring processor is suitable and
*** can be allocated, the search can continue. Otherwise the search
*** fails at this level.
*** Note that the processor ownership held in the network is out of date,
*** because at the very least some of the processors which used to be
*** free have been obtained by this very program.
**/

static bool	add_to_pipeline(RmNetwork result, ProcessorVector *vector,
			int position, int max)
{ RmProcessor	previous	= vector[position - 1].Template;
  int		number_links	= RmCountLinks(previous);
  int		i, j, destlink;
  RmProcessor	neighbour;

  if (position == max) return(TRUE);
  
  for (i = 0; i < number_links; i++)
   { neighbour = RmFollowLink(previous, i, &destlink);
     if ((neighbour == RmM_NoProcessor) ||
         (neighbour == RmM_ExternalProcessor))
      continue;

     if ((RmGetProcessorMemory(neighbour)		<  0x100000)   ||
#ifdef __C40
 	 (RmGetProcessorType(neighbour)			!= RmT_C40)    ||
#else
 	 (RmGetProcessorType(neighbour)			!= RmT_T800)   ||
#endif
         ((RmGetProcessorPurpose(neighbour) & RmP_Mask)	!= RmP_Helios) ||
         (RmGetProcessorOwner(neighbour)		!= RmO_FreePool))
        {
#if 0
          debug( "processor %s is unsuitable for addition to the pipline",
		RmGetProcessorId( neighbour ) );
#endif
          continue;
        }

     for (j = 0; j < position; j++)
      if (neighbour == vector[j].Template)
       goto skip;

     vector[position].Template = neighbour;
     vector[position].Obtained = RmObtainProcessor(neighbour);
     if (vector[position].Obtained == (RmProcessor) NULL)
      continue;

     RmAddtailProcessor(result, vector[position].Obtained);
     if (add_to_pipeline(result, vector, position + 1, max))
      return(TRUE);
     RmRemoveProcessor(vector[position].Obtained);
     RmReleaseProcessor(vector[position].Obtained);
     RmFreeProcessor(vector[position].Obtained);
skip:
     continue;
   }

  return(FALSE);        

} /* add_to_pipeline */

/**
*** Once all the required processors have been obtained it is possible
*** to map the taskforce onto the processors.
**/
static void map_taskforce(ProcessorVector *vector, RmTaskforce taskforce)
{ RmTask	task = RmFirstTask(taskforce);
  int		i = 0;
  
  for ( ; task != (RmTask) NULL; task = RmNextTask(task))
   { fprintf(stderr, "Mapped task %s onto processor %s\n", RmGetTaskId(task),
   		RmGetProcessorId(vector[i].Obtained));
     RmMapTask(vector[i++].Obtained, task);
   }
} /* map_taskforce */

static RmNetwork	obtain_network(RmTaskforce taskforce)
{ RmNetwork		result;
  ProcessorVector	*vector;
  int			number_processors;
  RmNetwork		whole_network;
  RmProcessor		starting_place;
  int                   starting_points = 0;

  number_processors = RmCountTasks(taskforce);

  vector = Malloc((word)number_processors * sizeof(ProcessorVector));
  result = RmNewNetwork();

  if ((result == (RmNetwork) NULL) || (vector == (ProcessorVector *) NULL))
    {
      debug( "out of memory whilst obtaining network" );

      return((RmNetwork) NULL);
    }

  RmSetNetworkId(result, "Obtained");
     
  whole_network = RmGetNetwork();
  
  if (whole_network == (RmNetwork) NULL)
    {
      debug( "unable to get network information" );

      return((RmNetwork) NULL);
    }

  if (RmCountProcessors(whole_network) < number_processors)
    {  
      fail( "not enough processors in the network." );
    }
   
  for (starting_place = (RmProcessor) RmSearchNetwork(whole_network, &find_starting_place);
       starting_place != (RmProcessor) NULL; 
       starting_place = (RmProcessor) RmSearchNetwork(whole_network, &find_starting_place))
    {
      vector[0].Template	= starting_place;
      vector[0].Obtained = RmObtainProcessor(starting_place);
      if (vector[0].Obtained == (RmProcessor) NULL) continue;
      RmAddtailProcessor(result, vector[0].Obtained);
      ++starting_points;

      /* If this recursive call succeeds then the whole pipeline can	*/
      /* be mapped.							*/

     if (add_to_pipeline(result, vector, 1, number_processors))
      { map_taskforce(vector, taskforce);
        return(result);
      }

      /* Failure, clean up and try another starting place	*/
      RmRemoveProcessor(vector[0].Obtained);
      RmReleaseProcessor(vector[0].Obtained);
      RmFreeProcessor(vector[0].Obtained);
    }

  if (starting_points)
    debug( "unable to find enough suitable processors for workers" );
  else
    debug( "unable to find any suitable processors at all" );

  return((RmNetwork) NULL);
  
} /* obtain_network */

/**----------------------------------------------------------------------------
*** Execute a taskforce through the Resource Management library.
**/
static int execute_taskforce(RmNetwork network, RmTaskforce taskforce)
{ RmTaskforce	running = RmExecuteTaskforce(network, taskforce, NULL);
  if (running == NULL)
   { fail( "failed to run taskforce.\n%s\n", RmMapErrorToString(RmErrno));
     return 0;
   }
  else
   { (void) RmWaitforTaskforce(running);
     return(RmGetTaskforceReturncode(running));
   }
} /* execute_taskforce */

int main(int argc, char **argv)
{ RmTaskforce	pipeline;
  RmNetwork	processors;
  int		number_tasks;

  if (argc != 2)
   { fail( "usage, mappipe <number of workers>" );
   }

  number_tasks = atoi(argv[1]);  

  pipeline = build_taskforce(number_tasks);
  
  if (pipeline == (RmTaskforce) NULL)
   { fail( "failed to build pipeline of tasks." );
   }
   
  processors = obtain_network(pipeline);

  if (processors == (RmNetwork) NULL)
   { fail( "failed to obtain a suitable pipeline of processors" );
   }

  return(execute_taskforce(processors, pipeline));
}

