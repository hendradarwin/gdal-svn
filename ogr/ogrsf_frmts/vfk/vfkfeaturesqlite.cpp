/******************************************************************************
 * $Id$
 *
 * Project:  VFK Reader - Feature definition (SQLite)
 * Purpose:  Implements VFKFeatureSQLite class.
 * Author:   Martin Landa, landa.martin gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2012-2013, Martin Landa <landa.martin gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ****************************************************************************/

#include "vfkreader.h"
#include "vfkreaderp.h"

#include "cpl_conv.h"
#include "cpl_error.h"

/*!
  \brief VFKFeatureSQLite constructor (from DB)

  Read VFK feature from DB

  \param poDataBlock pointer to related IVFKDataBlock
*/
VFKFeatureSQLite::VFKFeatureSQLite(IVFKDataBlock *poDataBlock) : IVFKFeature(poDataBlock)
{
    m_hStmt  = NULL;
    m_iRowId = m_poDataBlock->GetFeatureCount() + 1; /* starts at 1 */

    /* set FID from DB */
    SetFIDFromDB(); /* -> m_nFID */
}

/*!
  \brief VFKFeatureSQLite constructor 

  \param poDataBlock pointer to related IVFKDataBlock
  \param iRowId feature DB rowid (starts at 1)
  \param nFID feature id
*/
VFKFeatureSQLite::VFKFeatureSQLite(IVFKDataBlock *poDataBlock, int iRowId, long nFID) : IVFKFeature(poDataBlock)
{
    m_hStmt  = NULL;
    m_iRowId = iRowId;
    m_nFID   = nFID;
}

/*!
  \brief Read FID from DB
*/
OGRErr VFKFeatureSQLite::SetFIDFromDB()
{
    CPLString   osSQL;
    
    osSQL.Printf("SELECT %s FROM %s WHERE rowid = %d",
                 FID_COLUMN, m_poDataBlock->GetName(), m_iRowId);
    if (ExecuteSQL(osSQL.c_str()) != OGRERR_NONE)
        return OGRERR_FAILURE;

    m_nFID = sqlite3_column_int(m_hStmt, 0);
  
    FinalizeSQL();
    
    return OGRERR_NONE;
}

/*!
  \brief Set DB row id

  \param iRowId row id to be set
*/
void VFKFeatureSQLite::SetRowId(int iRowId)
{
    m_iRowId = iRowId;
}

/*!
  \brief Finalize SQL statement
*/
void VFKFeatureSQLite::FinalizeSQL()
{
    sqlite3_finalize(m_hStmt);
    m_hStmt = NULL;
}

/*!
  \brief Execute SQL (select) statement

  \param pszSQLCommand SQL command string

  \return OGRERR_NONE on success or OGRERR_FAILURE on error
*/
OGRErr VFKFeatureSQLite::ExecuteSQL(const char *pszSQLCommand)
{
    int rc;

    sqlite3  *poDB;
    
    VFKReaderSQLite *poReader = (VFKReaderSQLite *) m_poDataBlock->GetReader();
    poDB = poReader->m_poDB;
    
    rc = sqlite3_prepare(poDB, pszSQLCommand, strlen(pszSQLCommand),
                         &m_hStmt, NULL);
    if (rc != SQLITE_OK) {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "In ExecuteSQL(): sqlite3_prepare(%s):\n  %s",
                 pszSQLCommand, sqlite3_errmsg(poDB));
        
        if(m_hStmt != NULL) {
            FinalizeSQL();
        }
        return OGRERR_FAILURE;
    }
    rc = sqlite3_step(m_hStmt);
    if (rc != SQLITE_ROW) {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "In ExecuteSQL(): sqlite3_step(%s):\n  %s", 
                 pszSQLCommand, sqlite3_errmsg(poDB));
        
        if (m_hStmt) {
            FinalizeSQL();
        }
        
        return OGRERR_FAILURE;
    }

    return OGRERR_NONE;
}

/*!
  \brief VFKFeatureSQLite constructor (derived from VFKFeature)

  Read VFK feature from VFK file and insert it into DB
*/
VFKFeatureSQLite::VFKFeatureSQLite(const VFKFeature *poVFKFeature) : IVFKFeature(poVFKFeature->m_poDataBlock)
{
    m_nFID   = poVFKFeature->m_nFID;
    m_hStmt  = NULL;
    m_iRowId = m_poDataBlock->GetFeatureCount() + 1; /* starts at 1 */
}

/*!
  \brief Load geometry (point layers)

  \todo Implement (really needed?)
  
  \return TRUE on success or FALSE on failure
*/
bool VFKFeatureSQLite::LoadGeometryPoint()
{
    return FALSE;
}

/*!
  \brief Load geometry (linestring SBP layer)

  \todo Implement (really needed?)

  \return TRUE on success or FALSE on failure
*/
bool VFKFeatureSQLite::LoadGeometryLineStringSBP()
{
    return FALSE;
}

/*!
  \brief Load geometry (linestring HP/DPM layer)

  \todo Implement (really needed?)

  \return TRUE on success or FALSE on failure
*/
bool VFKFeatureSQLite::LoadGeometryLineStringHP()
{
    return FALSE;
}

/*!
  \brief Load geometry (polygon BUD/PAR layers)

  \todo Implement (really needed?)

  \return TRUE on success or FALSE on failure
*/
bool VFKFeatureSQLite::LoadGeometryPolygon()
{
    return FALSE;
}

/*!
  \brief Load feature properties from DB

  \param poFeature pointer to OGR feature

  \return OGRERR_NONE on success or OGRERR_FAILURE on failure
*/
OGRErr VFKFeatureSQLite::LoadProperties(OGRFeature *poFeature)
{
    CPLString   osSQL;

    osSQL.Printf("SELECT * FROM %s WHERE rowid = %d",
                 m_poDataBlock->GetName(), m_iRowId);
    if (ExecuteSQL(osSQL.c_str()) != OGRERR_NONE)
        return OGRERR_FAILURE;

    for (int iField = 0; iField < m_poDataBlock->GetPropertyCount(); iField++) {
	if (sqlite3_column_type(m_hStmt, iField) == SQLITE_NULL) /* skip null values */
            continue;
        OGRFieldType fType = poFeature->GetDefnRef()->GetFieldDefn(iField)->GetType();
        if (fType == OFTInteger)
            poFeature->SetField(iField,
                                sqlite3_column_int(m_hStmt, iField));
        else if (fType == OFTReal)
            poFeature->SetField(iField,
                                sqlite3_column_double(m_hStmt, iField));
        else
            poFeature->SetField(iField,
                                (const char *) sqlite3_column_text(m_hStmt, iField));
    }

    FinalizeSQL();

    return OGRERR_NONE;
}
