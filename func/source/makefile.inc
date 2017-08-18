_LNG=$(LANGUAGE)
_INX=.
STAMP=stampinf -f $@ -a $(_BUILDARCH) -k $(KMDF_VERSION_MAJOR).$(KMDF_VERSION_MINOR)


$(OBJ_PATH)\$(O)\$(INF_NAME).inf: $(_INX)\$(INF_NAME).inx
        copy $(_INX)\$(@B).inx $@
        $(STAMP)

$(OBJ_PATH)\$(O)\vmon_func.bmf : vmon_func.mof
    mofcomp -B:$@ vmon_func.mof
    wmimofck $@   
    wmimofck -m -h$(OBJ_PATH)\$O\vmon_func.h -w$(OBJ_PATH)\$O\htm $@

