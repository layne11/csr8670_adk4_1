try:
    from string import atoi
    from re import split
    from xide import app

    def versions():
        '''Return a dictionary specifying version information for xIDE
        and any currently loaded plugins'''

        tupleFromMakefile = ("BlueLab","7.1","Release")

        dict = {}

        if tupleFromMakefile[0] == 'Firmware':
            (_,major,minor,letter,_) = split('([0-9]*).([0-9]*)(.*)',tupleFromMakefile[1])
            dict['xide'] = ('Firmware',atoi(major),atoi(minor),letter)

        if tupleFromMakefile[0] == 'BlueLab':
            (_,version,suffix) = tupleFromMakefile
            dict['xide'] = ('BlueLab',version,suffix)

        if (app.debugger):
            for p in app.debugger.processors:
                if (p.extension.pluginNameAndVersion):
                    (name,versionstring) = p.extension.pluginNameAndVersion().split(",")
                    (major,minor) = versionstring.split(".")
                    if "-" in minor:
                        (minor,patch) = minor.split("-")
                        dict[name] = (atoi(major),atoi(minor),atoi(patch))
                    else:
                        dict[name] = (atoi(major),atoi(minor))

        return dict;
except:
   def versions():
       None
