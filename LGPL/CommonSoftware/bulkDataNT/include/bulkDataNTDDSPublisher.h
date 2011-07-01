#ifndef _BULK_DATA_NT_DDS_PUBLISHER_H_
#define _BULK_DATA_NT_DDS_PUBLISHER_H_
/*******************************************************************************
* ALMA - Atacama Large Millimiter Array
* (c) European Southern Observatory, 2011
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*
* "@(#) $Id: bulkDataNTDDSPublisher.h,v 1.1 2011/05/20 13:39:23 bjeram Exp $"
*
* who       when      what
* --------  --------  ----------------------------------------------
* bjeram  2011-04-19  created
*/

/************************************************************************
 *
 *----------------------------------------------------------------------
 */

#ifndef __cplusplus
#error This is a C++ include file and cannot be used from plain C
#endif

#include "bulkDataNTDDS.h"

namespace AcsBulkdata
{

/**
 * class responsible for all DDS Publisher related details
 */

class BulkDataNTDDSPublisher : public BulkDataNTDDS
{
public:

	/**
	 * Constructor
	 */
	BulkDataNTDDSPublisher();

	/**
	 * Destructor
	 */
	virtual ~BulkDataNTDDSPublisher();

protected:
	DDS::Publisher* createDDSPublisher(); // should return publisher ?
	//should return generic writer and have another method in Base class that narrows
	ACSBulkData::BulkDataNTFrameDataWriter* createDDSWriter(DDS::Publisher* pub, DDS::Topic *topic);

};//class BulkDataNTDDSPublisher

};

#endif