#ifndef __GEOCLUE_POSITION_ERROR_H__
#define __GEOCLUE_POSITION_ERROR_H__



#define GEOCLUE_POSITION_ERROR geoclue_position_error_quark ()

GQuark geoclue_position_error_quark (void);

/* Error codes for position backends */
typedef enum
{
  GEOCLUE_POSITION_ERROR_NOSERVICE,     /* Backend cannot connect to needed service */
  GEOCLUE_POSITION_ERROR_MALFORMEDDATA, /* Received data, but it is unreadable */
  GEOCLUE_POSITION_ERROR_NODATA,        /* Used service cannot provide position data at this time*/
  GEOCLUE_POSITION_ERROR_NOTSUPPORTED,  /* Backend does not implement this method */
  GEOCLUE_POSITION_ERROR_FAILED         /* generic fatal error */
} GeocluePositionError;


typedef enum position_status
{
    GEOCLUE_POSITION_NO_SERVICE_AVAILABLE    = 0,  
    GEOCLUE_POSITION_ACQUIRING_ALTITUDE      = 1,  
    GEOCLUE_POSITION_ACQUIRING_LONGITUDE     = 2,  
    GEOCLUE_POSITION_ACQUIRING_LATITUDE      = 4,  
    GEOCLUE_POSITION_ALTITUDE_AVAILABLE      = 8,  
    GEOCLUE_POSITION_LONGITUDE_AVAILABLE     = 16,  
    GEOCLUE_POSITION_LATITUDE_AVAILABLE      = 32     
  
} position_status;

#endif /* __GEOCLUE_POSITION_ERROR_H__ */
