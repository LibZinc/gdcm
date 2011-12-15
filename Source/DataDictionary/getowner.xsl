<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="text"/>
<!-- XSL to extract XML GDCM2 data owner -->
<!--
  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL: https://gdcm.svn.sourceforge.net/svnroot/gdcm/tags/gdcm-2-0-12/Source/DataDictionary/getowner.xsl $

  Copyright (c) 2006-2009 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
-->
  <xsl:template match="/">
<!-- The main template that loop over all dict/entry -->
    <xsl:for-each select="dict/entry">
      <xsl:text>{"</xsl:text>
      <xsl:value-of select="@owner"/>
      <xsl:text>","</xsl:text>
      <xsl:value-of select="@version"/>
      <xsl:text>"},</xsl:text>
      <xsl:text>
</xsl:text>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
