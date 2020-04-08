FROM ubuntu
WORKDIR /root/
COPY ./build/game ./
copy random_first.txt /root
copy random_last.txt /root
EXPOSE 8899
cmd ["./game","debug"]
