SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BAZEL=bazel-4.1.0

tmux new -d -s bitcode "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //bitcode:main"
tmux new -d -s eesier "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //eesi:main"
tmux new -d -s embedding  "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //embedding:service"
tmux new -d -s walker "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //walker:main"
tmux new -d -s getgraph "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //getgraph:main"
tmux new -d -s checker "cd ${SCRIPT_DIR}/.. && ${BAZEL} run //checker:main"
mkdir -p ~/data/db
tmux new -d -s mongo "mongod --dbpath ~/data/db"
