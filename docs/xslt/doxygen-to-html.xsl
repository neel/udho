<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="html" encoding="UTF-8" indent="yes"
      doctype-system="about:legacy-compat"/>
  <xsl:strip-space elements="*"/>
  <xsl:param name="selected-module" select="''"/>
  <xsl:param name="selected-compound" select="''"/>
  <xsl:param name="selected-compound-key" select="''"/>
  <xsl:param name="selected-owner-ref" select="''"/>
  <xsl:param name="selected-owner-name" select="''"/>
  <xsl:param name="selected-member" select="''"/>
  <xsl:param name="page-type" select="'module'"/>
  <xsl:param name="index-only" select="'no'"/>

  <xsl:template match="/documentation">
    <xsl:variable name="manifest" select="."/>
    <html lang="en">
      <head>
        <meta charset="utf-8"/>
        <meta name="viewport" content="width=device-width, initial-scale=1"/>
        <title><xsl:value-of select="@project"/> reference</title>
        <link rel="stylesheet" href="udho.css"/>
      </head>
      <body>
        <header class="site-header">
          <div>
            <p class="eyebrow">C++ API reference</p>
            <h1><xsl:value-of select="@project"/></h1>
            <p>Experimental HTML generated directly from Doxygen XML with XSLT.</p>
          </div>
        </header>
        <xsl:choose>
          <xsl:when test="$index-only='yes'">
            <main class="landing">
              <h2>Modules</h2>
              <div class="module-cards">
                <xsl:for-each select="module">
                  <xsl:variable name="index" select="document(@index)/doxygenindex"/>
                  <a href="{@name}.html">
                    <strong><xsl:value-of select="@name"/></strong>
                    <span><xsl:value-of select="count($index/compound)"/> documented compounds</span>
                  </a>
                </xsl:for-each>
              </div>
            </main>
          </xsl:when>
          <xsl:otherwise>
            <div class="site-grid">
              <nav class="sidebar" aria-label="Module navigation">
                <strong>Modules</strong>
                <ul>
                  <xsl:for-each select="module">
                    <li><a href="{@name}.html"><xsl:value-of select="@name"/></a></li>
                  </xsl:for-each>
                </ul>
              </nav>
              <main>
                <xsl:for-each select="module[@name=$selected-module]">
                  <xsl:variable name="module" select="@name"/>
                  <xsl:variable name="base" select="substring-before(@index, 'index.xml')"/>
                  <xsl:variable name="index" select="document(@index)/doxygenindex"/>
                  <xsl:choose>
                    <xsl:when test="$page-type='member' or $page-type='free-member'">
                      <xsl:variable name="compound" select="document(concat($base, $selected-compound, '.xml'), $manifest)/doxygen/compounddef"/>
                      <article class="member-page">
                        <nav class="breadcrumbs">
                          <a href="{$module}.html"><xsl:value-of select="$module"/></a><span>/</span>
                          <a><xsl:attribute name="href"><xsl:choose><xsl:when test="$page-type='free-member'"><xsl:value-of select="concat($module, '-', $selected-owner-ref, '.html')"/></xsl:when><xsl:otherwise><xsl:value-of select="concat($module, '-', $selected-compound, '.html')"/></xsl:otherwise></xsl:choose></xsl:attribute><xsl:choose><xsl:when test="$page-type='free-member'"><xsl:value-of select="$selected-owner-name"/></xsl:when><xsl:otherwise><xsl:value-of select="$compound/compoundname"/></xsl:otherwise></xsl:choose></a><span>/</span>
                          <strong><xsl:value-of select="$compound//memberdef[@id=$selected-member]/name"/></strong>
                        </nav>
                        <header class="module-header member-page-header">
                          <p class="eyebrow"><xsl:choose><xsl:when test="$page-type='free-member'">Free function</xsl:when><xsl:otherwise>Member function</xsl:otherwise></xsl:choose></p>
                          <h2><xsl:choose><xsl:when test="$page-type='free-member'"><xsl:value-of select="concat($selected-owner-name, '::', $compound//memberdef[@id=$selected-member]/name)"/></xsl:when><xsl:otherwise><xsl:value-of select="$compound//memberdef[@id=$selected-member]/qualifiedname"/></xsl:otherwise></xsl:choose></h2>
                        </header>
                        <xsl:apply-templates select="$compound//memberdef[@id=$selected-member]"/>
                      </article>
                    </xsl:when>
                    <xsl:when test="$page-type='group'">
                      <xsl:call-template name="group-page">
                        <xsl:with-param name="group" select="document(concat($base, $selected-compound, '.xml'), $manifest)/doxygen/compounddef"/>
                        <xsl:with-param name="index" select="$index"/>
                        <xsl:with-param name="module" select="$module"/>
                        <xsl:with-param name="base" select="$base"/>
                      </xsl:call-template>
                    </xsl:when>
                    <xsl:when test="$page-type='namespace'">
                      <xsl:call-template name="namespace-page">
                        <xsl:with-param name="namespace" select="document(concat($base, $selected-compound, '.xml'), $manifest)/doxygen/compounddef"/>
                        <xsl:with-param name="index" select="$index"/>
                        <xsl:with-param name="module" select="$module"/>
                        <xsl:with-param name="base" select="$base"/>
                      </xsl:call-template>
                    </xsl:when>
                    <xsl:when test="$page-type='directory'">
                      <xsl:call-template name="directory-page">
                        <xsl:with-param name="directory" select="document(concat($base, $selected-compound, '.xml'), $manifest)/doxygen/compounddef"/>
                        <xsl:with-param name="index" select="$index"/>
                        <xsl:with-param name="module" select="$module"/>
                        <xsl:with-param name="base" select="$base"/>
                      </xsl:call-template>
                    </xsl:when>
                    <xsl:when test="$page-type='compound' or $page-type='file'">
                      <xsl:apply-templates select="document(concat($base, $selected-compound, '.xml'), $manifest)/doxygen/compounddef">
                        <xsl:with-param name="module" select="$module"/>
                      </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:variable name="root-group-ref" select="concat('group__DoxyG__', $module)"/>
                      <xsl:variable name="root-group" select="document(concat($base, $root-group-ref, '.xml'), $manifest)/doxygen/compounddef"/>
                      <section class="module" id="module-{@name}">
                        <header class="module-header">
                          <p class="eyebrow">Module</p>
                          <h2><xsl:value-of select="@name"/></h2>
                          <p><xsl:value-of select="count($index/compound)"/> documented compounds</p>
                        </header>
                        <div class="typed-index">
                          <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Groups'"/><xsl:with-param name="items" select="$index/compound[@kind='group' and @refid=$root-group/innergroup/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Namespaces'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace' and name=concat('udho::', $module)]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Classes'"/><xsl:with-param name="items" select="$index/compound[(@kind='class' or @kind='struct' or @kind='union') and @refid=$root-group/innerclass/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Functions'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace']/member[@kind='function' and @refid=$root-group/sectiondef/memberdef[@kind='function']/@id]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Globals'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace']/member[@kind='variable' and @refid=$root-group/sectiondef/memberdef[@kind='variable']/@id]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Typedefs'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace']/member[@kind='typedef' and @refid=$root-group/sectiondef/memberdef[@kind='typedef']/@id]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Enumerations'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace']/member[@kind='enum' and @refid=$root-group/sectiondef/memberdef[@kind='enum']/@id]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                          <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Pages'"/><xsl:with-param name="items" select="$index/compound[@kind='page']"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
                        </div>
                        <section class="module-details">
                          <h2>Module documentation</h2>
                          <xsl:for-each select="$index/compound[not(@kind='class' or @kind='struct' or @kind='union' or @kind='group' or @kind='file' or @kind='namespace' or @kind='dir')]">
                            <xsl:apply-templates select="document(concat($base, @refid, '.xml'), $manifest)/doxygen/compounddef">
                              <xsl:with-param name="module" select="$module"/>
                            </xsl:apply-templates>
                          </xsl:for-each>
                        </section>
                      </section>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:for-each>
              </main>
              <xsl:for-each select="module[@name=$selected-module]">
                <xsl:variable name="module" select="@name"/>
                <xsl:variable name="base" select="substring-before(@index, 'index.xml')"/>
                <xsl:variable name="index" select="document(@index)/doxygenindex"/>
                <xsl:variable name="root-group-ref" select="concat('group__DoxyG__', $module)"/>
                <xsl:variable name="root-directory" select="$index/compound[@kind='dir' and name='udho'][1]"/>
                <aside class="context-sidebar" aria-label="Documentation hierarchy">
                  <xsl:if test="$index/compound[@kind='group' and @refid=$root-group-ref]"><section><h2>Group hierarchy</h2><ul class="group-tree"><xsl:call-template name="group-tree-node"><xsl:with-param name="refid" select="$root-group-ref"/><xsl:with-param name="base" select="$base"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="manifest" select="$manifest"/></xsl:call-template></ul></section></xsl:if>
                  <xsl:if test="$root-directory"><section><h2>Directories</h2><ul class="directory-tree"><xsl:call-template name="directory-tree-node"><xsl:with-param name="refid" select="$root-directory/@refid"/><xsl:with-param name="base" select="$base"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="manifest" select="$manifest"/></xsl:call-template></ul></section></xsl:if>
                </aside>
              </xsl:for-each>
            </div>
          </xsl:otherwise>
        </xsl:choose>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="directory-page">
    <xsl:param name="directory"/>
    <xsl:param name="index"/>
    <xsl:param name="module"/>
    <xsl:param name="base"/>
    <article class="directory-page" id="{$directory/@id}">
      <nav class="breadcrumbs"><a href="{$module}.html"><xsl:value-of select="$module"/></a><span>/</span><strong><xsl:value-of select="$directory/compoundname"/></strong></nav>
      <header class="module-header"><p class="eyebrow">Directory</p><h2><xsl:value-of select="$directory/compoundname"/></h2></header>
      <xsl:apply-templates select="$directory/briefdescription|$directory/detaileddescription"/>
      <div class="typed-index">
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Directories'"/><xsl:with-param name="items" select="$index/compound[@kind='dir' and @refid=$directory/innerdir/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Files'"/><xsl:with-param name="items" select="$index/compound[@kind='file' and @refid=$directory/innerfile/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
      </div>
      <xsl:apply-templates select="$directory/location"/>
    </article>
  </xsl:template>

  <xsl:template name="namespace-page">
    <xsl:param name="namespace"/>
    <xsl:param name="index"/>
    <xsl:param name="module"/>
    <xsl:param name="base"/>
    <xsl:variable name="namespace-index" select="$index/compound[@kind='namespace' and @refid=$namespace/@id]"/>
    <article class="namespace-page" id="{$namespace/@id}">
      <nav class="breadcrumbs"><a href="{$module}.html"><xsl:value-of select="$module"/></a><span>/</span><strong><xsl:value-of select="$namespace/compoundname"/></strong></nav>
      <header class="module-header"><p class="eyebrow">Namespace</p><h2><xsl:value-of select="$namespace/compoundname"/></h2></header>
      <xsl:apply-templates select="$namespace/briefdescription|$namespace/detaileddescription"/>
      <div class="typed-index">
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Namespaces'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace' and @refid=$namespace/innernamespace/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Classes'"/><xsl:with-param name="items" select="$index/compound[(@kind='class' or @kind='struct' or @kind='union') and @refid=$namespace/innerclass/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Functions'"/><xsl:with-param name="items" select="$namespace-index/member[@kind='function']"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Globals'"/><xsl:with-param name="items" select="$namespace-index/member[@kind='variable']"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Typedefs'"/><xsl:with-param name="items" select="$namespace-index/member[@kind='typedef']"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="member-overview-section"><xsl:with-param name="title" select="'Enumerations'"/><xsl:with-param name="items" select="$namespace-index/member[@kind='enum']"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Groups'"/><xsl:with-param name="items" select="$index/compound[@kind='group' and @refid=$namespace/innergroup/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
      </div>
      <xsl:apply-templates select="$namespace/sectiondef"/>
      <xsl:apply-templates select="$namespace/location"/>
    </article>
  </xsl:template>

  <xsl:template name="group-tree-node">
    <xsl:param name="refid"/>
    <xsl:param name="base"/>
    <xsl:param name="module"/>
    <xsl:param name="manifest"/>
    <xsl:variable name="group" select="document(concat($base, $refid, '.xml'), $manifest)/doxygen/compounddef"/>
    <li><a href="{$module}-{$refid}.html"><xsl:if test="$page-type='group' and $selected-compound=$refid"><xsl:attribute name="class">current</xsl:attribute></xsl:if><xsl:value-of select="$group/title"/></a>
      <xsl:if test="$group/innergroup"><ul><xsl:for-each select="$group/innergroup"><xsl:sort select="."/><xsl:call-template name="group-tree-node"><xsl:with-param name="refid" select="@refid"/><xsl:with-param name="base" select="$base"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="manifest" select="$manifest"/></xsl:call-template></xsl:for-each></ul></xsl:if>
    </li>
  </xsl:template>

  <xsl:template name="directory-tree-node">
    <xsl:param name="refid"/>
    <xsl:param name="base"/>
    <xsl:param name="module"/>
    <xsl:param name="manifest"/>
    <xsl:variable name="directory" select="document(concat($base, $refid, '.xml'), $manifest)/doxygen/compounddef"/>
    <li><a href="{$module}-{$refid}.html"><xsl:if test="$page-type='directory' and $selected-compound=$refid"><xsl:attribute name="class">current</xsl:attribute></xsl:if><code><xsl:call-template name="path-basename"><xsl:with-param name="path" select="$directory/compoundname"/></xsl:call-template></code></a>
      <xsl:if test="$directory/innerdir"><ul><xsl:for-each select="$directory/innerdir"><xsl:sort select="."/><xsl:call-template name="directory-tree-node"><xsl:with-param name="refid" select="@refid"/><xsl:with-param name="base" select="$base"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="manifest" select="$manifest"/></xsl:call-template></xsl:for-each></ul></xsl:if>
    </li>
  </xsl:template>

  <xsl:template name="path-basename">
    <xsl:param name="path"/>
    <xsl:choose><xsl:when test="contains($path, '/')"><xsl:call-template name="path-basename"><xsl:with-param name="path" select="substring-after($path, '/')"/></xsl:call-template></xsl:when><xsl:otherwise><xsl:value-of select="$path"/></xsl:otherwise></xsl:choose>
  </xsl:template>

  <xsl:template name="group-page">
    <xsl:param name="group"/>
    <xsl:param name="index"/>
    <xsl:param name="module"/>
    <xsl:param name="base"/>
    <article class="group-page" id="{$group/@id}">
      <nav class="breadcrumbs"><a href="{$module}.html"><xsl:value-of select="$module"/></a><span>/</span><strong><xsl:value-of select="$group/title"/></strong></nav>
      <header class="module-header"><p class="eyebrow">Group</p><h2><xsl:value-of select="$group/title"/></h2></header>
      <xsl:apply-templates select="$group/briefdescription|$group/detaileddescription"/>
      <div class="typed-index">
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Groups'"/><xsl:with-param name="items" select="$index/compound[@kind='group' and @refid=$group/innergroup/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Namespaces'"/><xsl:with-param name="items" select="$index/compound[@kind='namespace' and @refid=$group/innernamespace/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="compound-overview-section"><xsl:with-param name="title" select="'Classes'"/><xsl:with-param name="items" select="$index/compound[(@kind='class' or @kind='struct' or @kind='union') and @refid=$group/innerclass/@refid]"/><xsl:with-param name="module" select="$module"/><xsl:with-param name="base" select="$base"/></xsl:call-template>
        <xsl:call-template name="group-member-overview-section"><xsl:with-param name="title" select="'Functions'"/><xsl:with-param name="items" select="$group/sectiondef/memberdef[@kind='function' and @id=$index/compound[@kind='namespace']/member[@kind='function']/@refid]"/><xsl:with-param name="index" select="$index"/><xsl:with-param name="module" select="$module"/></xsl:call-template>
        <xsl:call-template name="group-member-overview-section"><xsl:with-param name="title" select="'Globals'"/><xsl:with-param name="items" select="$group/sectiondef/memberdef[@kind='variable' and @id=$index/compound[@kind='namespace']/member[@kind='variable']/@refid]"/><xsl:with-param name="index" select="$index"/><xsl:with-param name="module" select="$module"/></xsl:call-template>
        <xsl:call-template name="group-member-overview-section"><xsl:with-param name="title" select="'Typedefs'"/><xsl:with-param name="items" select="$group/sectiondef/memberdef[@kind='typedef' and @id=$index/compound[@kind='namespace']/member[@kind='typedef']/@refid]"/><xsl:with-param name="index" select="$index"/><xsl:with-param name="module" select="$module"/></xsl:call-template>
        <xsl:call-template name="group-member-overview-section"><xsl:with-param name="title" select="'Enumerations'"/><xsl:with-param name="items" select="$group/sectiondef/memberdef[@kind='enum' and @id=$index/compound[@kind='namespace']/member[@kind='enum']/@refid]"/><xsl:with-param name="index" select="$index"/><xsl:with-param name="module" select="$module"/></xsl:call-template>
      </div>
      <xsl:apply-templates select="$group/sectiondef"/>
      <xsl:apply-templates select="$group/location"/>
    </article>
  </xsl:template>

  <xsl:template name="group-member-overview-section">
    <xsl:param name="title"/>
    <xsl:param name="items"/>
    <xsl:param name="index"/>
    <xsl:param name="module"/>
    <xsl:if test="$items">
      <details open="open"><xsl:attribute name="class">overview-section<xsl:call-template name="category-class-for-title"><xsl:with-param name="title" select="$title"/></xsl:call-template></xsl:attribute><summary><span><xsl:value-of select="$title"/></span><small><xsl:value-of select="count($items)"/></small></summary>
        <table class="overview-table"><tbody>
          <xsl:for-each select="$items"><xsl:sort select="name"/><xsl:variable name="member-id" select="@id"/><xsl:variable name="owner-compound" select="$index/compound[@kind='namespace' and member/@refid=$member-id][1]"/><xsl:variable name="owner" select="$owner-compound/@refid"/>
            <tr><td><a><xsl:attribute name="href"><xsl:choose><xsl:when test="@kind='function' and $owner"><xsl:value-of select="concat($module, '-free-', $owner, '-', substring(@id, string-length(@id) - 32), '.html')"/></xsl:when><xsl:otherwise><xsl:value-of select="concat('#', @id)"/></xsl:otherwise></xsl:choose></xsl:attribute><code><xsl:choose><xsl:when test="@kind='function' and $owner-compound/name"><xsl:value-of select="concat($owner-compound/name, '::', name)"/></xsl:when><xsl:otherwise><xsl:value-of select="name"/></xsl:otherwise></xsl:choose></code></a></td><td><xsl:apply-templates select="briefdescription/node()"/></td></tr>
          </xsl:for-each>
        </tbody></table>
      </details>
    </xsl:if>
  </xsl:template>

  <xsl:template name="compound-overview-section">
    <xsl:param name="title"/>
    <xsl:param name="items"/>
    <xsl:param name="module"/>
    <xsl:param name="base"/>
    <xsl:if test="$items">
      <details open="open"><xsl:attribute name="class">overview-section<xsl:call-template name="category-class-for-title"><xsl:with-param name="title" select="$title"/></xsl:call-template></xsl:attribute>
        <summary><span><xsl:value-of select="$title"/></span><small><xsl:value-of select="count($items)"/></small></summary>
        <table class="overview-table"><tbody>
          <xsl:for-each select="$items">
            <xsl:sort select="name"/>
            <xsl:variable name="detail" select="document(concat(@refid, '.xml'), .)/doxygen/compounddef"/>
            <tr><td><a><xsl:attribute name="href"><xsl:choose><xsl:when test="@kind='class' or @kind='struct' or @kind='union' or @kind='group' or @kind='file' or @kind='namespace' or @kind='dir'"><xsl:value-of select="concat($module, '-', @refid, '.html')"/></xsl:when><xsl:otherwise><xsl:value-of select="concat('#', @refid)"/></xsl:otherwise></xsl:choose></xsl:attribute><code><xsl:choose><xsl:when test="@kind='group' and $detail/title"><xsl:value-of select="$detail/title"/></xsl:when><xsl:otherwise><xsl:value-of select="name"/></xsl:otherwise></xsl:choose></code></a></td><td><xsl:apply-templates select="$detail/briefdescription/node()"/></td></tr>
          </xsl:for-each>
          </tbody>
        </table>
      </details>
    </xsl:if>
  </xsl:template>

  <xsl:template name="member-overview-section">
    <xsl:param name="title"/>
    <xsl:param name="items"/>
    <xsl:param name="module"/>
    <xsl:param name="base"/>
    <xsl:if test="$items">
      <details open="open"><xsl:attribute name="class">overview-section<xsl:call-template name="category-class-for-title"><xsl:with-param name="title" select="$title"/></xsl:call-template></xsl:attribute>
        <summary><span><xsl:value-of select="$title"/></span><small><xsl:value-of select="count($items)"/></small></summary>
        <table class="overview-table"><tbody>
          <xsl:for-each select="$items">
            <xsl:sort select="name"/>
            <xsl:variable name="compound-ref" select="../@refid"/>
            <xsl:variable name="member-ref" select="@refid"/>
            <xsl:variable name="member-compound" select="substring-before($member-ref, concat('_1', substring(substring-after($member-ref, '_1'), 1, 1)))"/>
            <xsl:variable name="detail" select="document(concat($member-compound, '.xml'), .)/doxygen/compounddef//memberdef[@id=$member-ref]"/>
            <tr><td><a><xsl:attribute name="href"><xsl:choose><xsl:when test="@kind='function'"><xsl:value-of select="concat($module, '-free-', ../@refid, '-', substring(@refid, string-length(@refid) - 32), '.html')"/></xsl:when><xsl:otherwise><xsl:value-of select="concat($module, '-', ../@refid, '.html#', @refid)"/></xsl:otherwise></xsl:choose></xsl:attribute><code><xsl:choose><xsl:when test="@kind='function'"><xsl:value-of select="concat(../name, '::', name)"/></xsl:when><xsl:otherwise><xsl:value-of select="name"/></xsl:otherwise></xsl:choose></code></a></td><td><xsl:apply-templates select="$detail/briefdescription/node()"/></td></tr>
          </xsl:for-each>
          </tbody>
        </table>
      </details>
    </xsl:if>
  </xsl:template>

  <xsl:template name="category-class-for-title">
    <xsl:param name="title"/>
    <xsl:choose>
      <xsl:when test="$title='Typedefs' or $title='Member typedefs'"><xsl:text> category-typedef</xsl:text></xsl:when>
      <xsl:when test="$title='Globals' or $title='Variables' or $title='Member variables'"><xsl:text> category-variable</xsl:text></xsl:when>
      <xsl:when test="$title='Enumerations' or $title='Member enumerations'"><xsl:text> category-enum</xsl:text></xsl:when>
      <xsl:when test="$title='Macros'"><xsl:text> category-macro</xsl:text></xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="category-class-for-kind">
    <xsl:param name="kind"/>
    <xsl:choose>
      <xsl:when test="$kind='typedef'"><xsl:text> category-typedef</xsl:text></xsl:when>
      <xsl:when test="$kind='variable' or $kind='property' or $kind='event'"><xsl:text> category-variable</xsl:text></xsl:when>
      <xsl:when test="$kind='enum'"><xsl:text> category-enum</xsl:text></xsl:when>
      <xsl:when test="$kind='define'"><xsl:text> category-macro</xsl:text></xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="compounddef">
    <xsl:param name="module"/>
    <article class="compound page-{$page-type}" id="{@id}">
      <header class="compound-header">
        <div>
          <span class="kind"><xsl:value-of select="@kind"/></span>
          <span class="access"><xsl:value-of select="@prot"/></span>
          <h3><xsl:choose><xsl:when test="@kind='group' and title"><xsl:value-of select="title"/></xsl:when><xsl:otherwise><xsl:value-of select="compoundname"/></xsl:otherwise></xsl:choose></h3>
        </div>
        <a class="back" href="{$module}.html">Back to <xsl:value-of select="$module"/></a>
      </header>

      <xsl:if test="templateparamlist/param">
        <div class="template-declaration"><xsl:call-template name="template-parameters"/></div>
      </xsl:if>

      <xsl:if test="basecompoundref or derivedcompoundref">
        <div class="relationships">
          <xsl:if test="basecompoundref">
            <div><strong>Inherits</strong><xsl:apply-templates select="basecompoundref" mode="relation"/></div>
          </xsl:if>
          <xsl:if test="derivedcompoundref">
            <div><strong>Inherited by</strong><xsl:apply-templates select="derivedcompoundref" mode="relation"/></div>
          </xsl:if>
        </div>
      </xsl:if>

      <xsl:if test="includes or includedby">
        <dl class="metadata">
          <xsl:if test="includes"><dt>Include</dt><dd><xsl:apply-templates select="includes" mode="comma-list"/></dd></xsl:if>
          <xsl:if test="includedby"><dt>Included by</dt><dd><xsl:apply-templates select="includedby" mode="comma-list"/></dd></xsl:if>
        </dl>
      </xsl:if>

      <xsl:apply-templates select="briefdescription"/>
      <xsl:if test="$page-type='compound'">
        <input class="private-toggle-control" type="checkbox" id="show-private-members"/>
        <xsl:call-template name="declaration-table"/>
      </xsl:if>
      <xsl:apply-templates select="detaileddescription"/>

      <xsl:if test="innerclass or innernamespace or innergroup or innerdir or innerfile">
        <details class="inner-compounds" open="open">
          <summary>Nested and related compounds</summary>
          <ul><xsl:apply-templates select="innerclass|innernamespace|innergroup|innerdir|innerfile" mode="inner"/></ul>
        </details>
      </xsl:if>

      <xsl:apply-templates select="sectiondef"/>
      <xsl:apply-templates select="listofallmembers"/>
      <xsl:apply-templates select="programlisting"/>
      <xsl:apply-templates select="location"/>
    </article>
  </xsl:template>

  <xsl:template name="declaration-table">
    <section class="declarations">
      <div class="declarations-heading">
        <h2>Declarations</h2>
        <label class="private-toggle" for="show-private-members"><span class="private-toggle-switch" aria-hidden="true"></span><span>Show privates</span></label>
      </div>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Member typedefs'"/><xsl:with-param name="items" select="sectiondef/memberdef[@kind='typedef']"/></xsl:call-template>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Member enumerations'"/><xsl:with-param name="items" select="sectiondef/memberdef[@kind='enum']"/></xsl:call-template>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Member variables'"/><xsl:with-param name="items" select="sectiondef/memberdef[@kind='variable' or @kind='property' or @kind='event']"/><xsl:with-param name="collapsed" select="'yes'"/></xsl:call-template>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Member functions'"/><xsl:with-param name="items" select="sectiondef/memberdef[(@kind='function' or @kind='signal' or @kind='slot') and contains(definition, concat('::', name))]"/></xsl:call-template>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Friends'"/><xsl:with-param name="items" select="sectiondef/memberdef[@kind='friend']"/></xsl:call-template>
      <xsl:call-template name="declaration-group"><xsl:with-param name="title" select="'Macros'"/><xsl:with-param name="items" select="sectiondef/memberdef[@kind='define']"/></xsl:call-template>
      <xsl:if test="not(sectiondef/memberdef)"><p class="empty">No declarations.</p></xsl:if>
    </section>
  </xsl:template>

  <xsl:template name="declaration-group">
    <xsl:param name="title"/>
    <xsl:param name="items"/>
    <xsl:param name="collapsed" select="'no'"/>
    <xsl:if test="$items">
      <details><xsl:attribute name="class">declaration-group-block<xsl:if test="count($items[@prot='private'])=count($items)"> private-only</xsl:if><xsl:call-template name="category-class-for-kind"><xsl:with-param name="kind" select="$items[1]/@kind"/></xsl:call-template></xsl:attribute><xsl:if test="$collapsed!='yes'"><xsl:attribute name="open">open</xsl:attribute></xsl:if>
        <summary><span><xsl:value-of select="$title"/></span><small><xsl:value-of select="count($items)"/></small></summary>
        <table class="declaration-table"><tbody><xsl:for-each select="$items"><xsl:sort select="name"/><xsl:apply-templates select="." mode="declaration-row"/></xsl:for-each></tbody></table>
      </details>
    </xsl:if>
  </xsl:template>

  <xsl:template match="memberdef" mode="declaration-row">
    <tr class="access-{@prot}">
      <td><a class="declaration-name"><xsl:attribute name="href"><xsl:choose><xsl:when test="@kind='function' or @kind='signal' or @kind='slot'"><xsl:value-of select="concat($selected-module, '-member-', $selected-compound-key, '-', substring(@id, string-length(@id) - 32), '.html')"/></xsl:when><xsl:otherwise><xsl:value-of select="concat('#', @id)"/></xsl:otherwise></xsl:choose></xsl:attribute><xsl:value-of select="name"/></a><div class="declaration-badges"><span><xsl:value-of select="@prot"/></span><xsl:if test="@static='yes'"><span>static</span></xsl:if><xsl:if test="@const='yes'"><span>const</span></xsl:if></div></td>
      <td><xsl:if test="templateparamlist/param"><div class="declaration-template"><xsl:call-template name="template-parameters"/></div></xsl:if><code class="declaration-signature"><xsl:call-template name="member-signature"/></code></td>
      <td class="declaration-description"><xsl:apply-templates select="briefdescription/node()"/></td>
    </tr>
  </xsl:template>

  <xsl:template match="sectiondef">
    <xsl:if test="not($page-type='compound' or $page-type='group' or $page-type='namespace') or memberdef[not(@kind='function' or @kind='signal' or @kind='slot')]">
      <section><xsl:attribute name="class">member-section section-<xsl:value-of select="@kind"/><xsl:if test="starts-with(@kind, 'private')"> access-private</xsl:if><xsl:choose><xsl:when test="@kind='typedef'"> category-typedef</xsl:when><xsl:when test="@kind='var' or contains(@kind, 'attrib')"> category-variable</xsl:when><xsl:when test="@kind='enum'"> category-enum</xsl:when><xsl:when test="@kind='define'"> category-macro</xsl:when></xsl:choose></xsl:attribute>
      <h4>
        <xsl:choose>
          <xsl:when test="header"><xsl:value-of select="header"/></xsl:when>
          <xsl:otherwise><xsl:value-of select="translate(@kind, '-', ' ')"/></xsl:otherwise>
        </xsl:choose>
      </h4>
      <xsl:apply-templates select="description"/>
        <div class="members">
          <xsl:choose>
            <xsl:when test="$page-type='compound' or $page-type='group' or $page-type='namespace'"><xsl:apply-templates select="memberdef[not(@kind='function' or @kind='signal' or @kind='slot')]"/></xsl:when>
            <xsl:otherwise><xsl:apply-templates select="memberdef"/></xsl:otherwise>
          </xsl:choose>
        </div>
      </section>
    </xsl:if>
  </xsl:template>

  <xsl:template match="memberdef">
    <article id="{@id}"><xsl:attribute name="class">member member-<xsl:value-of select="@kind"/> access-<xsl:value-of select="@prot"/><xsl:call-template name="category-class-for-kind"><xsl:with-param name="kind" select="@kind"/></xsl:call-template></xsl:attribute>
      <header class="member-summary">
        <div class="member-identity">
          <span class="kind"><xsl:value-of select="@kind"/></span>
          <a href="#{@id}" class="member-name"><xsl:value-of select="name"/></a>
        </div>
        <div class="brief"><xsl:apply-templates select="briefdescription/node()"/></div>
      </header>
      <div class="member-detail">
        <xsl:if test="templateparamlist/param">
          <div class="template-declaration"><xsl:call-template name="template-parameters"/></div>
        </xsl:if>
        <pre class="signature"><xsl:call-template name="member-signature"/></pre>
        <div class="badges">
          <span><xsl:value-of select="@prot"/></span>
          <xsl:if test="@static='yes'"><span>static</span></xsl:if>
          <xsl:if test="@inline='yes'"><span>inline</span></xsl:if>
          <xsl:if test="@explicit='yes'"><span>explicit</span></xsl:if>
          <xsl:if test="@const='yes'"><span>const</span></xsl:if>
          <xsl:if test="@constexpr='yes'"><span>constexpr</span></xsl:if>
          <xsl:if test="@consteval='yes'"><span>consteval</span></xsl:if>
          <xsl:if test="@virt and @virt!='non-virtual'"><span><xsl:value-of select="@virt"/></span></xsl:if>
          <xsl:if test="@mutable='yes'"><span>mutable</span></xsl:if>
        </div>
        <xsl:if test="initializer"><div class="initializer"><strong>Initializer</strong><code><xsl:apply-templates select="initializer/node()"/></code></div></xsl:if>
        <xsl:if test="requiresclause"><div class="requires"><strong>Requires</strong><code><xsl:apply-templates select="requiresclause/node()"/></code></div></xsl:if>
        <xsl:apply-templates select="detaileddescription"/>
        <xsl:apply-templates select="inbodydescription"/>
        <xsl:if test="enumvalue">
          <table class="enum-values"><thead><tr><th>Enumerator</th><th>Value</th><th>Description</th></tr></thead>
            <tbody><xsl:apply-templates select="enumvalue"/></tbody>
          </table>
        </xsl:if>
        <xsl:apply-templates select="references|referencedby"/>
        <xsl:apply-templates select="location"/>
      </div>
    </article>
  </xsl:template>

  <xsl:template name="member-signature">
    <xsl:choose>
      <xsl:when test="@kind='function' or @kind='friend' or @kind='signal' or @kind='slot'">
        <xsl:apply-templates select="type/node()"/><xsl:text> </xsl:text>
        <strong><xsl:value-of select="name"/></strong><xsl:value-of select="argsstring"/>
      </xsl:when>
      <xsl:when test="@kind='typedef'">
        <xsl:text>using </xsl:text><strong><xsl:value-of select="name"/></strong><xsl:text> = </xsl:text><xsl:apply-templates select="type/node()"/>
      </xsl:when>
      <xsl:when test="@kind='enum'">
        <xsl:text>enum </xsl:text><xsl:if test="@strong='yes'"><xsl:text>class </xsl:text></xsl:if><strong><xsl:value-of select="name"/></strong>
      </xsl:when>
      <xsl:when test="@kind='define'">
        <xsl:text>#define </xsl:text><strong><xsl:value-of select="name"/></strong><xsl:if test="param"><xsl:text>(</xsl:text><xsl:for-each select="param"><xsl:if test="position()!=1">, </xsl:if><xsl:value-of select="defname"/></xsl:for-each><xsl:text>)</xsl:text></xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="type/node()"/><xsl:text> </xsl:text><strong><xsl:value-of select="name"/></strong><xsl:if test="argsstring"><xsl:value-of select="argsstring"/></xsl:if><xsl:if test="bitfield"><xsl:text> : </xsl:text><xsl:value-of select="bitfield"/></xsl:if>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="template-parameters">
    <span>template &lt;</span>
    <xsl:for-each select="templateparamlist/param">
      <xsl:if test="position()!=1"><xsl:text>, </xsl:text></xsl:if>
      <xsl:apply-templates select="type/node()"/>
      <xsl:if test="declname"><xsl:text> </xsl:text><xsl:value-of select="declname"/></xsl:if>
      <xsl:if test="defval"><xsl:text> = </xsl:text><xsl:apply-templates select="defval/node()"/></xsl:if>
    </xsl:for-each>
    <span>&gt;</span>
  </xsl:template>

  <xsl:template match="enumvalue">
    <tr id="{@id}"><td><code><xsl:value-of select="name"/></code></td><td><code><xsl:apply-templates select="initializer/node()"/></code></td><td><xsl:apply-templates select="briefdescription|detaileddescription"/></td></tr>
  </xsl:template>

  <xsl:template match="briefdescription|detaileddescription|inbodydescription|description">
    <div class="description"><xsl:apply-templates/></div>
  </xsl:template>

  <xsl:template match="para"><p><xsl:apply-templates/></p></xsl:template>
  <xsl:template match="bold"><strong><xsl:apply-templates/></strong></xsl:template>
  <xsl:template match="emphasis"><em><xsl:apply-templates/></em></xsl:template>
  <xsl:template match="computeroutput"><code><xsl:apply-templates/></code></xsl:template>
  <xsl:template match="subscript"><sub><xsl:apply-templates/></sub></xsl:template>
  <xsl:template match="superscript"><sup><xsl:apply-templates/></sup></xsl:template>
  <xsl:template match="strike"><s><xsl:apply-templates/></s></xsl:template>
  <xsl:template match="underline"><u><xsl:apply-templates/></u></xsl:template>
  <xsl:template match="small"><small><xsl:apply-templates/></small></xsl:template>
  <xsl:template match="linebreak"><br/></xsl:template>
  <xsl:template match="nonbreakablespace"><xsl:text>&#160;</xsl:text></xsl:template>
  <xsl:template match="sp"><xsl:text> </xsl:text></xsl:template>
  <xsl:template match="ref"><a href="#{@refid}"><xsl:apply-templates/></a></xsl:template>
  <xsl:template match="ulink"><a href="{@url}"><xsl:apply-templates/></a></xsl:template>
  <xsl:template match="anchor"><span id="{@id}"></span></xsl:template>
  <xsl:template match="formula"><code class="formula"><xsl:value-of select="."/></code></xsl:template>
  <xsl:template match="verbatim"><pre><xsl:value-of select="."/></pre></xsl:template>
  <xsl:template match="blockquote"><blockquote><xsl:apply-templates/></blockquote></xsl:template>

  <xsl:template match="itemizedlist"><ul><xsl:apply-templates/></ul></xsl:template>
  <xsl:template match="orderedlist"><ol><xsl:apply-templates/></ol></xsl:template>
  <xsl:template match="listitem"><li><xsl:apply-templates/></li></xsl:template>
  <xsl:template match="variablelist"><dl><xsl:apply-templates/></dl></xsl:template>
  <xsl:template match="varlistentry"><dt><xsl:apply-templates/></dt></xsl:template>
  <xsl:template match="term"><xsl:apply-templates/></xsl:template>
  <xsl:template match="listitem[parent::variablelist]"><dd><xsl:apply-templates/></dd></xsl:template>

  <xsl:template match="simplesect">
    <aside class="admonition {@kind}"><strong><xsl:value-of select="@kind"/></strong><xsl:apply-templates/></aside>
  </xsl:template>
  <xsl:template match="xrefsect">
    <aside class="admonition xref"><strong><xsl:value-of select="xreftitle"/></strong><xsl:apply-templates select="xrefdescription"/></aside>
  </xsl:template>

  <xsl:template match="parameterlist">
    <section class="parameters"><h5><xsl:value-of select="@kind"/> parameters</h5><dl><xsl:apply-templates/></dl></section>
  </xsl:template>
  <xsl:template match="parameteritem">
    <dt><xsl:for-each select="parameternamelist/parametername"><xsl:if test="position()!=1">, </xsl:if><code><xsl:value-of select="."/></code><xsl:if test="@direction"><small> [<xsl:value-of select="@direction"/>]</small></xsl:if></xsl:for-each></dt>
    <dd><xsl:apply-templates select="parameterdescription/node()"/></dd>
  </xsl:template>
  <xsl:template match="parameterdescription|parameternamelist"/>

  <xsl:template match="sect1|sect2|sect3|sect4|sect5|sect6">
    <section id="{@id}" class="document-section">
      <xsl:element name="h{count(ancestor::sect1|ancestor::sect2|ancestor::sect3|ancestor::sect4|ancestor::sect5|ancestor::sect6) + 4}"><xsl:value-of select="title"/></xsl:element>
      <xsl:apply-templates select="node()[not(self::title)]"/>
    </section>
  </xsl:template>

  <xsl:template match="table">
    <table class="doc-table"><xsl:if test="caption"><caption><xsl:apply-templates select="caption/node()"/></caption></xsl:if><tbody><xsl:apply-templates select="row"/></tbody></table>
  </xsl:template>
  <xsl:template match="row"><tr><xsl:apply-templates/></tr></xsl:template>
  <xsl:template match="entry"><td><xsl:if test="@thead='yes'"><xsl:attribute name="class">table-heading</xsl:attribute></xsl:if><xsl:if test="@colspan"><xsl:attribute name="colspan"><xsl:value-of select="@colspan"/></xsl:attribute></xsl:if><xsl:if test="@rowspan"><xsl:attribute name="rowspan"><xsl:value-of select="@rowspan"/></xsl:attribute></xsl:if><xsl:apply-templates/></td></xsl:template>

  <xsl:template match="programlisting"><pre class="programlisting"><xsl:apply-templates/></pre></xsl:template>
  <xsl:template match="codeline"><span class="code-line"><xsl:apply-templates/></span><xsl:text>&#10;</xsl:text></xsl:template>
  <xsl:template match="highlight"><span class="hl-{@class}"><xsl:apply-templates/></span></xsl:template>

  <xsl:template match="image"><figure><img src="{@name}" alt="{@name}"/><xsl:if test="normalize-space(.)"><figcaption><xsl:apply-templates/></figcaption></xsl:if></figure></xsl:template>
  <xsl:template match="dot|msc|plantuml"><pre class="diagram-source"><xsl:value-of select="."/></pre></xsl:template>
  <xsl:template match="dotfile|mscfile|diafile"><p class="diagram-file"><strong><xsl:value-of select="name()"/></strong>: <xsl:value-of select="."/></p></xsl:template>

  <xsl:template match="references|referencedby">
    <p class="reference"><strong><xsl:value-of select="name()"/>:</strong> <a href="#{@refid}"><xsl:value-of select="."/></a></p>
  </xsl:template>

  <xsl:template match="listofallmembers">
    <xsl:if test="$page-type!='compound'">
      <details class="all-members"><summary>All members</summary><ul><xsl:for-each select="member"><li><a href="#{@refid}"><xsl:value-of select="name"/></a><xsl:if test="scope"> — <xsl:value-of select="scope"/></xsl:if></li></xsl:for-each></ul></details>
    </xsl:if>
  </xsl:template>

  <xsl:template match="location">
    <p class="location"><strong>Source:</strong> <code><xsl:value-of select="@file"/><xsl:if test="@line">:<xsl:value-of select="@line"/></xsl:if><xsl:if test="@column">:<xsl:value-of select="@column"/></xsl:if></code><xsl:if test="@bodyfile"> · definition <code><xsl:value-of select="@bodyfile"/>:<xsl:value-of select="@bodystart"/>–<xsl:value-of select="@bodyend"/></code></xsl:if></p>
  </xsl:template>

  <xsl:template match="basecompoundref|derivedcompoundref" mode="relation">
    <a href="#{@refid}"><xsl:value-of select="."/></a><small><xsl:value-of select="@prot"/> <xsl:value-of select="@virt"/></small>
  </xsl:template>
  <xsl:template match="includes|includedby" mode="comma-list"><xsl:if test="position()!=1">, </xsl:if><code><xsl:value-of select="."/></code></xsl:template>
  <xsl:template match="innerclass|innernamespace|innergroup|innerdir|innerfile" mode="inner"><li><span class="kind"><xsl:value-of select="substring-after(name(), 'inner')"/></span><a href="{$selected-module}-{@refid}.html"><xsl:value-of select="."/></a></li></xsl:template>

  <!-- Preserve the text and links of less common Doxygen XML constructs. This
       fallback intentionally unwraps unknown structural elements instead of
       silently discarding their documented content. -->
  <xsl:template match="heading"><strong class="heading"><xsl:apply-templates/></strong></xsl:template>
  <xsl:template match="copydoc|details|xrefdescription"><xsl:apply-templates/></xsl:template>
  <xsl:template match="text()"><xsl:value-of select="."/></xsl:template>
  <xsl:template match="*"><xsl:apply-templates/></xsl:template>

</xsl:stylesheet>
