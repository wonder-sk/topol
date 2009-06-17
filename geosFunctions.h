#include <geos_c.h>
#include <qgsgeometry.h>

class GEOSException
{
  public:
    GEOSException( const char *theMsg )
    {
      if ( strcmp( theMsg, "Unknown exception thrown" ) == 0 && lastMsg )
      {
        delete [] theMsg;
        char *aMsg = new char[strlen( lastMsg )+1];
        strcpy( aMsg, lastMsg );
        msg = aMsg;
      }
      else
      {
        msg = theMsg;
        lastMsg = msg;
      }
    }

    // copy constructor
    GEOSException( const GEOSException &rhs )
    {
      *this = rhs;
    }

    ~GEOSException()
    {
      if ( lastMsg == msg )
        lastMsg = NULL;
      delete [] msg;
    }

    const char *what()
    {
      return msg;
    }

  private:
    const char *msg;
    static const char *lastMsg;
};

bool geosTouches(QgsGeometry* g1, QgsGeometry* g2)
{
  try
  {
    if (1 == GEOSTouches(g1->asGeos(), g2->asGeos()))
      return true;
  }
  catch (GEOSException &e) 
  {
    return false;
  }
}

bool geosOverlaps(QgsGeometry* g1, QgsGeometry* g2)
{
  try
  {
    if (1 == GEOSOverlaps(g1->asGeos(), g2->asGeos()))
      return true;
  }
  catch (GEOSException &e) 
  {
    return false;
  }
}

bool geosContains(QgsGeometry* g1, QgsGeometry* g2)
{
  try
  {
    if (1 == GEOSContains(g1->asGeos(), g2->asGeos()))
      return true;
  }
  catch (GEOSException &e) 
  {
    return false;
  }
}
