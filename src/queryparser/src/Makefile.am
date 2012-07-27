AM_LDFLAGS = -static-libtool-libs

lib_LTLIBRARIES=libqueryparser.la
libqueryparser_la_SOURCES=blender/QueryRewriter.cpp \
						  blender/TokenizerPool.cpp \
						  blender/KeywordRewriter.cpp \
						  blender/QueryParameterClassifier.cpp \
						  blender/QueryRewriterResult.cpp \
			  searcher/FilterParseInterface.cpp \
                          searcher/StringUtil.cpp \
                          searcher/LessFilterParser.cpp \
                          searcher/MoreFilterParser.cpp \
                          searcher/LessMoreFilterParser.cpp \
                          searcher/KeyvalueFilterParser.cpp \
                          searcher/KeyvalueSyntaxParser.cpp \
                          searcher/KeyWordSyntaxParser.cpp \
                          searcher/MultivalueFilterParser.cpp \
                          searcher/MultivalueSyntaxParser.cpp \
                          searcher/OtherSyntaxParser.cpp \
                          searcher/Param.cpp \
                          searcher/qp_syntax_tree.cpp \
                          searcher/FilterList.cpp\
                          searcher/QPResult.cpp \
                          searcher/QueryParser.cpp \
                          $(NULL)

DEFS+=$(MACRO)
