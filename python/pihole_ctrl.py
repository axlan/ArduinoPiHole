import re
import sys
import urllib.parse

import requests


def pretty_print_POST(req):
    """
    At this point it is completely built and ready
    to be fired; it is "prepared".

    However pay attention at the formatting used in 
    this function because it is programmed to be pretty 
    printed and may differ from the actual request.
    """
    print('{}\n{}\r\n{}\r\n\r\n{}'.format(
        '-----------START-----------',
        req.method + ' ' + req.url,
        '\r\n'.join('{}: {}'.format(k, v) for k, v in req.headers.items()),
        req.body,
    ))

class PiHoleControl():
    headers = {'Content-type': 'application/x-www-form-urlencoded; charset=UTF-8'}

    def __get_token(self):
        # req = requests.Request("GET", f'http://{self.host}/admin/groups-domains.php?type=black')
        # prepped = self.session.prepare_request(req)
        # pretty_print_POST(prepped)
        resp = self.session.get(f'http://{self.host}/admin/groups-domains.php?type=black')
        lines = resp.text.splitlines()
        token = None
        token_re = re.compile(r'<div id="token" hidden>(.+)</div>')
        for line in lines:
            m = token_re.match(line)
            if m is not None:
                token = m.group(1)
        return urllib.parse.quote(token)

    def __init__(self, host, persistentlogin):
        self.session = requests.session()
        self.host = host
        login_cookie = {'persistentlogin': persistentlogin}
        requests.utils.add_dict_to_cookiejar(self.session.cookies, login_cookie)
        self.token = self.__get_token()

    def get_blacklist_group(self):
        data = f'action=get_domains&showtype=black&token={self.token}'
        resp = self.session.post(f'http://{self.host}/admin/scripts/pi-hole/php/groups.php', headers=PiHoleControl.headers, data=data)
        print(resp.headers.items())
        return resp.json()['data']

    def get_groups(self):
        data = f'action=get_groups&token={self.token}'
        # {"data":[{"id":0,"enabled":1,"name":"Default","date_added":1597376555,"date_modified":1597376555,"description":"The default group"},{"id":1,"enabled":1,"name":"fun","date_added":1597427317,"date_modified":1598488617,"description":null},{"id":3,"enabled":0,"name":"work","date_added":1597427368,"date_modified":1598488594,"description":null}]}
        resp = self.session.post(f'http://{self.host}/admin/scripts/pi-hole/php/groups.php', headers=PiHoleControl.headers, data=data)
        return resp.json()['data']

    def set_group_by_id(self, id, name, enable, desc=""):
        if enable:
            status = 1
        else:
            status = 0
        data = f'action=edit_group&id={id}&name={name}&desc={desc}&status={status}&token={self.token}'
        resp = self.session.post(f'http://{self.host}/admin/scripts/pi-hole/php/groups.php', headers=PiHoleControl.headers, data=data)
        print(resp.json())

    def set_group_by_name(self, name, enable):
        groups = self.get_groups()
        group = None
        for val in groups:
            if val['name'] == name:
                group = val
                break
        if group is None:
            print(f'Name not found matching comment: {name}')
            return
        self.set_group_by_id(group['id'], name, enable, group['description'])

    def set_blacklist_item_by_id(self, item_id, item_type, comment, enable):
        if enable:
            status = 1
        else:
            status = 0
        data = f'action=edit_domain&id={item_id}&type={item_type}&comment={comment}&status={status}&token={self.token}'
        self.session.post(f'http://{self.host}/admin/scripts/pi-hole/php/groups.php', headers=PiHoleControl.headers, data=data)

    def set_blacklist_item_by_comment(self, comment, enable):
        black_list = self.get_blacklist_group()
        domain = None
        for val in black_list:
            if val['comment'] == comment:
                domain = val
                break
        if domain is None:
            print(f'Domain not found matching comment: {comment}')
            return
        self.set_blacklist_item_by_id(domain['id'], domain['type'], comment, enable)

def main():
    config_file = sys.argv[1]

    host_re = re.compile(r'#define PI_HOLE_HOST "(.+)"')
    key_re = re.compile(r'#define PERSISTANT_KEY "(.+)"')
    with open(config_file) as fd:
        for line in fd.readlines():
            m = host_re.match(line)
            if m:
                host = m.group(1)
            m = key_re.match(line)
            if m:
                key = m.group(1)

    ctrl = PiHoleControl(host, key)
    ctrl.set_group_by_name('fun', False)

if __name__ == '__main__':
    main()



