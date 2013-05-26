#!/bin/csh
set total = 0
set colons = `grep ";" $1 | wc -l`
set openp = `grep "{" $1 | wc -l`
set closep = `grep "}" $1 | wc -l`
echo "semi colons: $colons, open parenthesis: $openp, close parenthesis: $closep"
@ total = $colons + $openp + $closep
echo "total: $total"
