#include <Dispatch/OpenCLFunctions.h>

#include <iostream>

#include <CL/cl.h>

/* Event Object APIs */
cl_int
clWaitForEvents(cl_uint             num_events,
		const cl_event *    event_list)
{
  return real_clWaitForEvents(num_events, event_list);
}

cl_int
clGetEventInfo(cl_event         event,
	       cl_event_info    param_name,
	       size_t           param_value_size,
	       void *           param_value,
	       size_t *         param_value_size_ret)
{
  return real_clGetEventInfo(event, param_name, param_value_size,
			     param_value, param_value_size_ret);
}

cl_event
clCreateUserEvent(cl_context    /* context */,
		  cl_int *      /* errcode_ret */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int
clRetainEvent(cl_event event)
{
  return real_clRetainEvent(event);
}

cl_int
clReleaseEvent(cl_event event)
{
  return real_clReleaseEvent(event);
}

cl_int
clSetUserEventStatus(cl_event   /* event */,
		     cl_int     /* execution_status */)
{
  std::cerr << "Error : function " << __FUNCTION__ << " not handled yet !\n";
  exit(EXIT_FAILURE);
}

cl_int
clSetEventCallback( cl_event    event,
		    cl_int      command_exec_callback_type,
		    void (CL_CALLBACK * pfn_notify)(cl_event, cl_int, void *),
		    void *      user_data)
{
  return real_clSetEventCallback(event, command_exec_callback_type, pfn_notify,
				 user_data);
}

/* Profiling APIs */
cl_int
clGetEventProfilingInfo(cl_event            event,
			cl_profiling_info   param_name,
			size_t              param_value_size,
			void *              param_value,
			size_t *            param_value_size_ret)
{
  return real_clGetEventProfilingInfo(event, param_name, param_value_size,
				      param_value, param_value_size_ret);
}
