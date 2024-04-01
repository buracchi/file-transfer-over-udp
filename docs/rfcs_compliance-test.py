class TestSuite:

    @staticmethod
    def test_false():
        return False

    @staticmethod
    def test_true():
        return True


def main():
    with open('rfcs-compliance-results.md', 'w', encoding='utf8') as output:
        output.write('# RFCs Compliance Matrix\n\n<table>\n')
        with open('rfcs-compliance-metadata.csv', 'r', encoding='utf8') as input:
            for line in input:
                if line.startswith('RFC'):
                    rfc = line.replace(';', '').strip()
                    output.write(f'\t<th colspan="3">{rfc}</th>\n')
                elif not line.startswith(';'):
                    section = line.replace(';', '').strip()
                    output.write(f'\t<tr>\n\t\t<td colspan="3">{section}\n\t</tr>\n')
                else:
                    test_idx = line.index(';') + 1
                    result_idx = line[test_idx:].index(';') + 1
                    test_description = line[test_idx:result_idx].strip()
                    test_name = line[result_idx + 1:].strip()
                    if not test_name in dir(TestSuite):
                        print(f'Test with description:\'{test_description}\' has invalid test name: \'{test_name}\'')
                        exit(1)
                    test_result = '✅' if getattr(TestSuite, test_name)() else '❌'
                    output.write(f'\t<tr>\n\t\t<td>\n\t\t<td>{test_description}\n\t\t<td>{test_result}\n\t</tr>\n')
        output.write('</table>\n')


if __name__ == '__main__':
    main()
