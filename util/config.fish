function l --description 'List entire contents of directory using long format'
  ls -lah $argv
end

function lt --description 'List (by time) entire contents of directory using long format'
  ls -lhrt $argv
end
