#!/bin/bash
set -eu

SCRIPTPATH="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd)"
MAC_FILE="${SCRIPTPATH}/mac.txt"

USAGE=$(
  cat <<EOF
Usage: $0 BM ALPHA_SHIFT SCHEDULER 

BM: DT | Occamy | ABM | Pushout
SCHEDULER: PQ | DRR
EOF
)

checkroot() {
  if [[ "${EUID}" -ne 0 ]]; then
    echo "You need to run this script as root" >&1
    exit 1
  fi
}

main() {
  checkroot
  if (($# < 3)); then
    echo "$USAGE"
    return 1
  fi
  local bm="$1"
  local bm_v=0
  case "$bm" in
  DT)
    bm_v=0
    ;;
  Occamy)
    bm_v=1
    ;;
  Pushout)
    bm_v=3
    ;;
  ABM)
    bm_v=2
    ;;
  *)
    echo "$0: unrecognized BM algorithm: $3"
    echo "$USAGE" >&2
    return 1
    ;;
  esac

  local alpha=$2
  local sch="PQ"

  case "$3" in
  PQ | pq)
    sch="PQ"
    ;;
  DRR | drr)
    sch="DRR"
    ;;
  *)
    echo "$0: unrecognized packet scheduling algorithm: $3"
    echo "$USAGE" >&2
    return 1
    ;;
  esac

  make -C "$SCRIPTPATH" -B "$sch"

  $SCRIPTPATH/build/dpdk-switch -c 0x55555555 -- -p 0xff -t $bm_v -a $alpha -m $MAC_FILE
}

main "$@"
