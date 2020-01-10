create database test;
use test;
show databases;
create table a(a int not null, b int, c varchar(20));
create table b(a int, b int, c date);
show tables;
desc a;
desc b;
alter table a add primary key(a);
alter table b add constraint fk foreign key (a) references a(a);
insert into a values(1, 2, '123'), (2, 5, 'asd'), (4, 6, 'asd');
insert into b values(1, 2, '23333'), (1, 5, 'gas'), (4, 6, 'sadeq');
select * from a;
select * from b;