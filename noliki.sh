state=" . │ . │ .
───┼───┼───
 . │ . │ .
───┼───┼───
 . │ . │ .
"

function is_end {
    if [[ "$state" =~ (x.{3}x.{3}x)|(x.{22}x.{22}x)|(x.{26}x.{26}x)|(x.{18}x.{18}x) ]]; then
        winner="x"
    elif [[ "$state" =~ (o.{3}o.{3}o)|(o.{22}o.{22}o)|(o.{26}o.{26}o)|(o.{18}o.{18}o) ]]; then
        winner="o"
    elif ! [[ "$state" =~ \. ]]; then
        echo "Draw!"
        return
    else
        echo ""
        return
    fi

    if [ "$winner" == "$obj" ]; then
        echo "You won!"
    else
        echo "You lost!"
    fi
}

function move {
    good=false
    while [ $good == false ]; do
        read -d " " -p "Your move: " x
        read y

        if ! [[ "$x" =~ ^[0-2]$ ]] || ! [[ "$y" =~ ^[0-2]$ ]]; then
            echo "Wrong input!"
            continue
        fi

        let strx=$x*4+1+$y*23
        if ! [ "${state:$strx:1}" == "." ]; then
            echo "Already taken!"
            continue
        fi

        good=true
    done

    state="${state:0:$strx}$obj${state:$(($strx+1))}"
}

function game_cycle {
    while true; do
        echo "$state"

        res=$(is_end)
        if ! [ "$res" == "" ]; then
            echo "$res"
            break
        fi

        move
        echo "$state"
        echo "$state" > krestiki

        res=$(is_end)
        if ! [ "$res" == "" ]; then
            echo "$res"
            break
        fi

        state=$(cat krestiki)
        clear
    done
}

echo "Trying to find existing server..."
mkfifo krestiki > /dev/null 2>&1
if [ $? == 0 ]; then
    echo "You are server now. Go find a friend to play with."

    echo "Let's a go!" > krestiki
    greet=$(cat krestiki)
    clear
    echo "$greet"
    echo "You are krestik."

    obj="x"
    game_cycle

    rm krestiki
    exit
fi

clear
cat krestiki
echo "Let's a go!" > krestiki
echo "You are nolik."

echo "$state"
state=$(cat krestiki)
clear

obj="o"
game_cycle
