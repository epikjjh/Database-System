use Pokemon;

#Number 1
select name
from Trainer
where hometown = 'Blue City';

#Number 2
select name
from Trainer
where hometown = 'Brown City' or hometown = 'Rainbow City';

#Number 3
select name, hometown
from Trainer
where name like 'a%' or name like 'e%' or name like 'i%' or name like 'o%' or name like 'u%';

#Number 4
select name
from Pokemon
where type = 'Water';

#Number 5
select distinct type
from Pokemon;

#Number 6
select name
from Pokemon
order by name asc;

#Number 7
select name
from Pokemon
where name like '%s';

#Number 8
select name
from Pokemon
where name like '%e%s';

#Number 9
select name
from Pokemon
where name like 'a%' or name like 'e%' or name like 'i%' or name like 'o%' or name like 'u%';

#Number 10
select type, count(*) as '#number' 
from Pokemon
group by type;

#Number 11
select nickname
from CatchedPokemon
order by level desc limit 3;

#Number 12
select avg(level)
from CatchedPokemon;

#Number 13
select max(level) - min(level)
from CatchedPokemon;

#Number 14
select count(*)
from Pokemon
where name between 'b' and 'e';

#Number 15
select count(*)
from Pokemon
where type not in ('Fire','Grass','Water','Electric');

#Number 16
select T.name as 'Owner name', P.name as 'Pokemon name', C.nickname as 'Pokemon nick name'
from Trainer as T, Pokemon as P, CatchedPokemon as C
where C.nickname like '% %' and C.owner_id = T.id and C.pid = P.id;

#Number 17
select T.name as 'Trainer name'
from Trainer as T, Pokemon as P, CatchedPokemon as C
where C.pid = P.id and P.type = 'Psychic' and C.owner_id = T.id;

#Number 18
select T.name, T.hometown
from Trainer as T, CatchedPokemon as C
where T.id = C.owner_id
group by T.name
order by avg(level) desc limit 3;    

#Number 19
select T.name, count(*) as '#Catched Pokemon'
from Trainer as T, CatchedPokemon as C
where C.owner_id = T.id
group by T.name
order by count(*) desc, T.name desc;

#Number 20
select P.name, C.level
from Gym as G, Trainer as T, CatchedPokemon as C, Pokemon as P
where G.leader_id = T.id and G.city = 'Sangnok City' and C.owner_id = T.id and C.pid = P.id
order by C.level asc;

#Number 21
select P.name, count(pid) as '#Catch'
from Pokemon as P left join CatchedPokemon as C on P.id = C.pid
group by P.name
order by count(pid) desc;

#Number 22
select P.name
from Pokemon as P left join Evolution as E on P.id = E.before_id
where P.id in (
    select E.after_id
    from Pokemon as P left join Evolution as E on P.id = E.before_id
    where E.before_id in (
        select E.after_id
        from Pokemon as P left join Evolution as E on P.id = E.before_id
        where P.name = 'Charmander' and P.id = E.before_id));

#Number 23
select distinct P.name
from CatchedPokemon as C left join Pokemon as P on C.pid = P.id
where C.pid <= 30
order by P.name asc;

#Number 24
select T.name, P.type
from Trainer as T, CatchedPokemon as C, Pokemon as P
where T.id = C.owner_id and C.pid = P.id
group by T.name
having count(distinct P.type) = 1;

#Number 25
select T.name, P.type, count(*) as '#number'
from Trainer as T, CatchedPokemon as C, Pokemon as P
where T.id = C.owner_id and C.pid = P.id
group by T.name, P.type;

#Number 26
select T.name as 'Trainer name', P.name as 'Pokemon name', count(P.name) as '#Catch'
from Trainer as T, CatchedPokemon as C, Pokemon as P
where T.id = C.owner_id and C.pid = P.id
group by T.name
having count(distinct P.name) = 1;

#Number 27
select T.name as 'Leader name', City.name as 'City name'
from Trainer as T, Gym as G, City
where G.leader_id = T.id and G.city = City.name and City.name = T.hometown and T.name not in (
    select T.name 
    from Trainer as T, Gym as G, CatchedPokemon as C, Pokemon as P
    where G.leader_id = T.id and T.id = C.owner_id and C.pid = P.id
    group by T.name
    having count(distinct P.type) = 1);

#Number 28
select T.name, sum(if(C.level >= 50, C.level, NULL)) as 'sum'
from Gym as G, Trainer as T, CatchedPokemon as C
where G.leader_id = T.id and T.id = C.owner_id
group by T.name;

#Number 29
select distinct P.name
from CatchedPokemon as C left join Pokemon as P on C.pid = P.id
where (C.pid in (
        select C.pid
        from CatchedPokemon as C left join Trainer as T on T.id = C.owner_id
        where T.hometown = 'Blue city')
   and C.pid in (
        select C.pid
        from CatchedPokemon as C left join Trainer as T on T.id = C.owner_id
        where T.hometown = 'Sangnok City'));

#Number 30
select P.name
from Pokemon as P left join Evolution as E on P.id = E.before_id
where E.before_id is NULL and E.after_id is NULL;
