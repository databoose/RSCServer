#!/bin/bash

if time make; then
    echo "======================"
    echo "(bash) build successful"
    echo "======================"
    ./RSC_Server
else
    echo -e "\e[31m(bash) failed build\e[0m"
fi
