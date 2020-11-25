#!/bin/bash
if ["%(ls -A obj"]; then
    cd obj;
    find . -name "*.o" -exec mv "{}" ../ && pwd \;
    cd ../ && pwd;
else
    echo "(bash) obj folder is empty, generating fresh objects";
fi

if time make; then
    echo "======================"
    echo "(bash) build successful"
    echo "======================"
    ./RSC_Server
else
    echo -e "\e[31m(bash) failed build\e[0m"
fi

find . -name "*.o" -exec mv "{}" obj \;